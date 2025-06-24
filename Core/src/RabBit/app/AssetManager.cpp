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

    LoadedMesh::LoadedMesh()
        : internalScene(nullptr)
    {}

    LoadedMesh::~LoadedMesh()
    {
        ufbx_free_scene((ufbx_scene*)internalScene);
    }

    namespace AssetManager
    {
        static const char* g_AssetPath = nullptr;

        void Init(const char* asset_path)
        {
            g_AssetPath = asset_path;
        }

        // ---------------------------------------------------------------------------
        //								    Images
        // ---------------------------------------------------------------------------

        bool LoadImage8Bit(const char* path, LoadedImage* out_image, uint32_t force_channels)
        {
            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            RB_LOG(LOGTAG_MAIN, "Loading: %s", final_path.c_str());

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

        // ---------------------------------------------------------------------------
        //								    Meshes
        // ---------------------------------------------------------------------------

        LoadedMesh::Submodel ConvertMeshPart(const ufbx_mesh* mesh, const ufbx_mesh_part* mesh_part);

        bool LoadMesh(const char* path, LoadedMesh* out_mesh)
        {
            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            RB_LOG(LOGTAG_MAIN, "Loading: %s", final_path.c_str());

            auto file_handle = FileLoader::OpenFile(final_path.c_str(), OpenFileMode::kFileMode_Read | OpenFileMode::kFileMode_Binary);

            FileData data = file_handle->ReadFull();

            ufbx_load_opts opts = { };
            opts.target_axes                    = ufbx_axes_right_handed_y_up;
            opts.space_conversion               = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS; // Good for Blender, use UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY for Maya
            opts.target_unit_meters             = 1.0f;
            opts.geometry_transform_handling    = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES;

            ufbx_error error;
            out_mesh->internalScene = ufbx_load_memory(data.data, data.size, &opts, &error);

            if (out_mesh->internalScene == nullptr)
            {
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Failed to load model \"%s\" with ufbx, error message: %s", final_path.c_str(), error.description.data);
                return false;
            }

            ufbx_scene* scene = (ufbx_scene*)out_mesh->internalScene;

            // TODO Convert materials
            //for (const ufbx_material* node : scene->materials)
            //{
            //    printf("%s\n", node->name.data);
            //}

            for (const ufbx_node* node : scene->nodes) 
            {
                ufbx_mesh* mesh = node->mesh;

                if (mesh == nullptr)
                    continue;

                for (const ufbx_mesh_part& mesh_part : mesh->material_parts)
                {
                    LoadedMesh::Submodel submodel = ConvertMeshPart(mesh, &mesh_part);
                    out_mesh->models.push_back(submodel);
                }
            }

            static_assert(false);
            // TODO 
            // - With indices generating the data is sometimes still wrong
            // - Every submodel probably also needs its own transform
            // - Model loading op een andere thread doen?
            //      - Je kan dan behaviour kiezen of je moet blocken of gwn kan skippen totdat hij is geladen

            return true;
        }
        
        LoadedMesh::Submodel ConvertMeshPart(const ufbx_mesh* mesh, const ufbx_mesh_part* mesh_part)
        {
            List<Math::Float3> vertices;
            List<Math::Float3> normals;
            List<Math::Float2> uvs;

            const size_t num_vertices = mesh_part->num_triangles * 3;
            vertices.reserve(num_vertices);
            normals.reserve(num_vertices);
            uvs.reserve(num_vertices);

            const size_t num_tri_indices = mesh->max_face_triangles * 3;
            uint32_t* tri_indices = ALLOC_HEAPC(uint32_t, num_tri_indices);

            // First fetch all vertices into a flat non-indexed buffer, we also need to triangulate the faces
            for (size_t fi = 0; fi < mesh_part->num_faces; fi++)
            {
                ufbx_face face = mesh->faces.data[mesh_part->face_indices.data[fi]];
                size_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

                ufbx_vec2 default_uv = { 0 };

                // Iterate through every vertex of every triangle in the triangulated result
                for (size_t vi = 0; vi < num_tris * 3; vi++) 
                {
                    uint32_t ix = tri_indices[vi];

                    ufbx_vec3 pos    = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
                    ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
                    ufbx_vec2 uv     = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, ix) : default_uv;

                    vertices.push_back(Math::Float3(pos.x, pos.y, pos.z));
                    normals.push_back(Math::Float3(normal.x, normal.y, normal.z));
                    uvs.push_back(Math::Float2(uv.x, uv.y));
                }
            }

            SAFE_FREE(tri_indices);
            RB_ASSERT(LOGTAG_MAIN, vertices.size() == num_vertices, "The amount of loaded vertices does not match what was expected");

            List<ufbx_vertex_stream> streams;

            {
                ufbx_vertex_stream vertex_stream;
                vertex_stream.data         = vertices.data();
                vertex_stream.vertex_count = vertices.size();
                vertex_stream.vertex_size  = sizeof(Math::Float3);

                streams.push_back(vertex_stream);
            }

            {
                ufbx_vertex_stream normal_stream;
                normal_stream.data         = normals.data();
                normal_stream.vertex_count = normals.size();
                normal_stream.vertex_size  = sizeof(Math::Float3);

                streams.push_back(normal_stream);
            }

            {
                ufbx_vertex_stream uv_stream;
                uv_stream.data         = uvs.data();
                uv_stream.vertex_count = uvs.size();
                uv_stream.vertex_size  = sizeof(Math::Float2);

                streams.push_back(uv_stream);
            }

            LoadedMesh::Submodel out_submodel = {};

            const size_t num_indices = num_vertices;

            List<uint32_t> indices;
            indices.resize(num_indices);

            // Optimize the flat vertex buffer into an indexed one. `ufbx_generate_indices()`
            // compacts the vertex buffer and returns the number of used vertices.
            ufbx_error error;
            const size_t num_compacted_vertices = ufbx_generate_indices(streams.data(), streams.size(), indices.data(), num_indices, nullptr, &error);
            if (error.type == UFBX_ERROR_NONE) 
            {
                out_submodel.indices.resize(num_indices);
                for (int i = 0; i < num_indices; i++)
                {
                    out_submodel.indices[i] = indices[i];
                }
            
                out_submodel.vertices.resize(num_compacted_vertices);
                for (int i = 0; i < num_compacted_vertices; i++)
                {
                    out_submodel.vertices[i].position = vertices[i];
                    out_submodel.vertices[i].normal   = normals[i];
                    out_submodel.vertices[i].uv       = uvs[i];
                }
            }
            else
            {
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Failed to generate index buffer with ufbx, error message: %s", error.description.data);

                // Return just without the indices
                out_submodel.vertices.resize(num_vertices);
                for (int i = 0; i < num_vertices; i++)
                {
                    out_submodel.vertices[i].position = vertices[i];
                    out_submodel.vertices[i].normal = normals[i];
                    out_submodel.vertices[i].uv = uvs[i];
                }
            }

            return out_submodel;
        }
}
}