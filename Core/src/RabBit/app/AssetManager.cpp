#include "RabBitCommon.h"
#include "AssetManager.h"
#include "utils/File.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ufbx.h>

using namespace RB::Graphics;

namespace RB
{
    LoadedImage::LoadedImage()
        : data(nullptr)
    {}

    LoadedImage::~LoadedImage()
    {
        stbi_image_free(data);
        data = nullptr;
    }

    LoadedModel::LoadedModel()
        : internalScene(nullptr)
        , vertices(nullptr)
        , indices(nullptr)
    {}

    LoadedModel::~LoadedModel()
    {
        SAFE_DELETE(vertices);
        SAFE_DELETE(indices);
        ufbx_free_scene((ufbx_scene*)internalScene);
    }

    namespace AssetManager
    {
        static const char* g_AssetPath = nullptr;

        void Init(const char* asset_path)
        {
            g_AssetPath = asset_path;
        }

        bool LoadImage8Bit(const char* path, LoadedImage* out_image, uint32_t force_channels)
        {
            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            auto file_handle = FileLoader::OpenFile(final_path.c_str(), OpenFileMode::kFileMode_Read | OpenFileMode::kFileMode_Binary);

            FileData data = file_handle->ReadFull();

            // Note that this loads a 8 bit per channel image (use stbi_load_16_from_memory or stbi_loadf_from_memory for 16 or 32 bit)
            out_image->data = stbi_load_from_memory((stbi_uc*)data.data, data.size, &out_image->width, &out_image->height, &out_image->channels, force_channels);

            if (out_image->data == NULL)
            {
                const char* error_msg = stbi_failure_reason();
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Failed to load texture \"%s\" with STB, error message: %s", final_path.c_str(), error_msg);
                return false;
            }

            if (force_channels != 0)
            {
                // We forced to read only certain channels
                out_image->channels = force_channels;
            }

            switch (out_image->channels)
            {
            case 1: out_image->format = RenderResourceFormat::R8_UNORM; break;
            case 4: out_image->format = RenderResourceFormat::R8G8B8A8_UNORM; break;
            case 0:
            case 2:
            case 3:
            default:
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "This many channels for an 8 bit image is not supported");
                out_image->format = RenderResourceFormat::Unkown;
                break;
            }

            out_image->dataSize = GetElementSizeFromFormat(out_image->format) * out_image->width * out_image->height;

            return true;
        }

        bool LoadModel(const char* path, LoadedModel* out_model)
        {
            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            auto file_handle = FileLoader::OpenFile(final_path.c_str(), OpenFileMode::kFileMode_Read | OpenFileMode::kFileMode_Binary);

            FileData data = file_handle->ReadFull();

            ufbx_load_opts opts = { };
            opts.target_axes            = ufbx_axes_right_handed_y_up;
            opts.target_unit_meters     = 1.0f;

            ufbx_error error;
            out_model->internalScene = ufbx_load_memory(data.data, data.size, &opts, &error);

            if (out_model->internalScene == nullptr)
            {
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Failed to load model \"%s\" with ufbx, error message: %s", final_path.c_str(), error.description.data);
                return false;
            }

            // Just only load the first model for now, we can implement all the rest later
            {
                ufbx_mesh* mesh = ((ufbx_scene*)out_model->internalScene)->meshes[0];

                size_t max_triangles = 0;
                for (size_t pi = 0; pi < mesh->materials.count; pi++) 
                {
                    ufbx_mesh_part* mesh_mat = &mesh->material_parts.data[pi];
                    max_triangles = Math::Max(max_triangles, mesh_mat->num_triangles);
                }

                out_model->vertices = ALLOC_HEAPC(LoadedModel::Vertex, max_triangles * 3);
                out_model->indices  = ALLOC_HEAPC(uint32_t, max_triangles * 3);

                size_t num_tri_indices = mesh->max_face_triangles * 3;
                uint32_t* tri_indices = ALLOC_HEAPC(uint32_t, num_tri_indices);

                RB_ASSERT(LOGTAG_GRAPHICS, mesh->materials.count == 1, "We only support 1 material per FBX right now");

                // Our shader supports only a single material per draw call so we need to split the mesh
                // into parts by material. `ufbx_mesh_material` contains a handy compact list of faces
                // that use the material which we use here.
                for (size_t pi = 0; pi < mesh->materials.count; pi++) 
                {
                    ufbx_mesh_part* mesh_mat = &mesh->material_parts.data[pi];

                    if (mesh_mat->num_triangles == 0) 
                        continue;

                    size_t num_indices = 0;

                    // First fetch all vertices into a flat non-indexed buffer, we also need to triangulate the faces
                    for (size_t fi = 0; fi < mesh_mat->num_faces; fi++) 
                    {
                        ufbx_face face = mesh->faces.data[mesh_mat->face_indices.data[fi]];
                        size_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

                        ufbx_vec2 default_uv = { 0 };

                        // Iterate through every vertex of every triangle in the triangulated result
                        for (size_t vi = 0; vi < num_tris * 3; vi++) 
                        {
                            uint32_t ix = tri_indices[vi];
                            LoadedModel::Vertex* vert = &out_model->vertices[num_indices];

                            ufbx_vec3 pos    = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
                            ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
                            ufbx_vec2 uv     = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, ix) : default_uv;

                            vert->position   = Math::Float3(pos.x, pos.y, pos.z);
                            vert->normal     = Math::Float3(normal.x, normal.y, normal.z);
                            vert->uv         = Math::Float2(uv.x, uv.y);
                            vert->normal.Normalize();

                            num_indices++;
                        }
                    }

                    // TODO Auto generation of the indices & compaction of the vertices is currently broken

                    //ufbx_vertex_stream stream;
                    //stream.data         = &out_model->vertices;
                    //stream.vertex_count = max_triangles * 3;
                    //stream.vertex_size  = sizeof(LoadedModel::Vertex);
                    //
                    //// Optimize the flat vertex buffer into an indexed one. `ufbx_generate_indices()`
                    //// compacts the vertex buffer and returns the number of used vertices.
                    //ufbx_error error;
                    //size_t num_vertices = ufbx_generate_indices(&stream, 1, out_model->indices, num_indices, NULL, &error);
                    //if (error.type != UFBX_ERROR_NONE) 
                    //{
                    //    RB_LOG_ERROR(LOGTAG_GRAPHICS, "Failed to generate index buffer for model \"%s\" with ufbx, error message: %s", final_path.c_str(), error.description.data);
                    //    SAFE_FREE(tri_indices);
                    //    return false;
                    //}
                    //
                    //out_model->verticesCount = num_vertices;
                    //out_model->indicesCount = num_indices;

                    out_model->verticesCount = max_triangles * 3;
                    out_model->indicesCount = 0;
                }
                
                SAFE_FREE(tri_indices);
            }

            return true;
        }
    }
}