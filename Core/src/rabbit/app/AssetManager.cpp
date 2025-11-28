#include "RabBitCommon.h"
#include "AssetManager.h"
#include "utils/File.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ufbx.h>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace RB::Graphics;

namespace RB
{
    LoadedImage::LoadedImage()
        : data(nullptr)
        , dataSize(0)
        , width(0)
        , height(0)
    {}

    LoadedImage::~LoadedImage()
    {
        if (loadedUsingStb)
        {
            stbi_image_free(data);
        }
        else
        {
            SAFE_FREE(data);
        }
    }

    LoadedMesh::LoadedMesh()
        : internalScene(nullptr)
    {}

    LoadedMesh::~LoadedMesh()
    {
        ufbx_free_scene((ufbx_scene*)internalScene);
    }

    LoadedFont::LoadedFont()
        : fontFace(nullptr)
        , fontLibrary(nullptr)
    {}

    LoadedFont::~LoadedFont()
    {
        if (fontFace)
            FT_Done_Face((FT_Face)fontFace);
        if (fontLibrary)
            FT_Done_FreeType((FT_Library)fontLibrary);
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

        bool LoadImage8Bit(const char* path, LoadedImage* out_image, bool srgb)
        {
            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            RB_LOG(LOGTAG_MAIN, "Loading image: %s", final_path.c_str());

            auto file_handle = FileLoader::OpenFile(final_path.c_str(), OpenFileMode::kFileMode_Read | OpenFileMode::kFileMode_Binary);

            if (!file_handle->IsValid())
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Failed to load texture file \"%s\" from disk", final_path.c_str());
                return false;
            }

            FileData data = file_handle->ReadFull();

            int32_t actual_channels;
            bool success = stbi_info_from_memory((stbi_uc*)data.data, data.size, &out_image->width, &out_image->height, &actual_channels);

            if (!success)
            {
                const char* error_msg = stbi_failure_reason();
                RB_LOG_ERROR(LOGTAG_MAIN, "Failed to load info of texture \"%s\" with STB, error message: %s", final_path.c_str(), error_msg);
            }

            if (actual_channels == 3)
                actual_channels = 4; // We don't support 3 channel alpha textures

            // Note that this loads a 8 bit per channel image (use stbi_load_16_from_memory or stbi_loadf_from_memory for 16 or 32 bit)
            int32_t original_channels;
            out_image->data = stbi_load_from_memory((stbi_uc*)data.data, data.size, &out_image->width, &out_image->height, &original_channels, actual_channels);
            out_image->loadedUsingStb = true;

            if (out_image->data == NULL)
            {
                const char* error_msg = stbi_failure_reason();
                RB_LOG_ERROR(LOGTAG_MAIN, "Failed to load texture \"%s\" with STB, error message: %s", final_path.c_str(), error_msg);
                return false;
            }

            switch (actual_channels)
            {
            case 1:
                out_image->format = RenderResourceFormat::R8_UNORM; 
                if (srgb)
                    RB_LOG_WARN(LOGTAG_MAIN, "A single channel image cannot be in srgb space");
                break;
            case 4:
                out_image->format = srgb ? RenderResourceFormat::R8G8B8A8_SRGB : RenderResourceFormat::R8G8B8A8_UNORM; 
                break;
            case 0:
            case 2:
            case 3:
            default:
                RB_LOG_ERROR(LOGTAG_MAIN, "This many channels for an 8 bit image is not supported");
                out_image->format = RenderResourceFormat::Unkown;
                break;
            }

            out_image->dataSize = GetElementSizeFromFormat(out_image->format) * out_image->width * out_image->height;

            return true;
        }

        // ---------------------------------------------------------------------------
        //								    Meshes
        // ---------------------------------------------------------------------------

        LoadedMesh::Submodel ConvertMeshPart(const ufbx_mesh* mesh, const ufbx_mesh_part* mesh_part, const ufbx_node* node);

        bool LoadMesh(const char* path, LoadedMesh* out_mesh)
        {
            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            RB_LOG(LOGTAG_MAIN, "Loading mesh: %s", final_path.c_str());

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
                RB_LOG_ERROR(LOGTAG_MAIN, "Failed to load model \"%s\" with ufbx, error message: %s", final_path.c_str(), error.description.data);
                return false;
            }

            RB_LOG(LOGTAG_MAIN, "Model loaded from memory");

            ufbx_scene* scene = (ufbx_scene*)out_mesh->internalScene;

            for (const ufbx_material* material : scene->materials)
            {
                out_mesh->albedoTextures.push_back(material->fbx.diffuse_color.texture ? material->fbx.diffuse_color.texture->filename.data : material->name.data);

                // Store the wrap type 
                //material->fbx.diffuse_color.texture->wrap_u
                //material->fbx.diffuse_color.texture->wrap_v
            }

            for (const ufbx_node* node : scene->nodes) 
            {
                ufbx_mesh* mesh = node->mesh;

                if (mesh == nullptr)
                    continue;

                for (uint32_t part_idx = 0; part_idx < mesh->material_parts.count; part_idx++)
                {
                    LoadedMesh::Submodel submodel = ConvertMeshPart(mesh, &mesh->material_parts[part_idx], node);
                    submodel.albedoIndex = part_idx;
                    out_mesh->models.push_back(submodel);
                }

            }

            //static_assert(false);
            // TODO
            // - Do proper parent/child relationships
            // - Do some scale conversions in here so that we don't need really small scales when rendering (can cause floating point issues in for examply frustum culling)
            // - Store more texture types (normals, roughness, etc.)
            // - Do model loading on a different thread?
            //      - You can then choose the behaviour when its not yet loaded. Need to block until loaded or just skip rendering until loaded?

            return true;
        }

        LoadedMesh::Submodel ConvertMeshPart(const ufbx_mesh* mesh, const ufbx_mesh_part* mesh_part, const ufbx_node* node)
        {
            const size_t num_vertices = mesh_part->num_triangles * 3;
            List<Math::Float3> positions;
            positions.resize(num_vertices);
            List<LoadedMesh::Vertex> vertices;
            vertices.resize(num_vertices);

            const size_t num_tri_indices = mesh->max_face_triangles * 3;
            if (num_tri_indices > 1000.0f)
                RB_LOG_WARN(LOGTAG_MAIN, "Triange indices might overflow, maybe need to use ALLOC_HEAP");
            uint32_t* tri_indices = ALLOC_STACKC(uint32_t, num_tri_indices);

            Math::Float3 min_bounds(+FLT_MAX, +FLT_MAX, +FLT_MAX);
            Math::Float3 max_bounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            uint32_t vi_global = 0;
            // First fetch all vertices into a flat non-indexed buffer, we also need to triangulate the faces
            for (size_t fi = 0; fi < mesh_part->num_faces; fi++)
            {
                ufbx_face face = mesh->faces.data[mesh_part->face_indices.data[fi]];
                size_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

                ufbx_vec3 default_normal = { 0, 0, 1 };
                ufbx_vec2 default_uv = { 0, 0 };

                // Iterate through every vertex of every triangle in the triangulated result
                for (size_t vi = 0; vi < num_tris * 3; vi++) 
                {
                    uint32_t ix = tri_indices[vi];

                    ufbx_vec3 pos    = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
                    ufbx_vec3 normal = mesh->vertex_normal.exists ? ufbx_get_vertex_vec3(&mesh->vertex_normal, ix) : default_normal;
                    ufbx_vec2 uv     = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, ix) : default_uv;

                    positions[vi_global] = Math::Float3(pos.x, pos.y, pos.z);

                    vertices[vi_global] = {};
                    vertices[vi_global].normal   = Math::Float3(normal.x, normal.y, normal.z);
                    vertices[vi_global].uv       = Math::Float2(uv.x, 1.0f - uv.y); // Flip the Y as UFBX uses bottom-left convention
                    vi_global++;

                    if (pos.x < min_bounds.x) min_bounds.x = pos.x;
                    if (pos.y < min_bounds.y) min_bounds.y = pos.y;
                    if (pos.z < min_bounds.z) min_bounds.z = pos.z;
                    if (pos.x > max_bounds.x) max_bounds.x = pos.x;
                    if (pos.y > max_bounds.y) max_bounds.y = pos.y;
                    if (pos.z > max_bounds.z) max_bounds.z = pos.z;
                }
            }

            RB_ASSERT(LOGTAG_MAIN, vertices.size() == num_vertices, "The amount of loaded vertices does not match what was expected");

            List<ufbx_vertex_stream> streams(2);
            streams[0].data = positions.data(); streams[0].vertex_count = positions.size(); streams[0].vertex_size = sizeof(Math::Float3);
            streams[1].data = vertices.data();  streams[1].vertex_count = vertices.size();  streams[1].vertex_size = sizeof(LoadedMesh::Vertex);

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
                memcpy(out_submodel.indices.data(), indices.data(), sizeof(uint32_t) * num_indices);

                out_submodel.positions.resize(num_compacted_vertices);
                memcpy(out_submodel.positions.data(), positions.data(), sizeof(Math::Float3) * num_compacted_vertices);

                out_submodel.vertices.resize(num_compacted_vertices);
                memcpy(out_submodel.vertices.data(), vertices.data(), sizeof(LoadedMesh::Vertex) * num_compacted_vertices);

                out_submodel.minBounds = min_bounds;
                out_submodel.maxBounds = max_bounds;
            }
            else
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Failed to generate index buffer with ufbx, error message: %s", error.description.data);
                // Return empty submodel
                return out_submodel;
            }

            // Transform code
            {
                out_submodel.position.x = (float)node->node_to_world.m03;
                out_submodel.position.y = (float)node->node_to_world.m13;
                out_submodel.position.z = (float)node->node_to_world.m23;

                const ufbx_matrix& m = node->node_to_world;

                Math::Float3 cx(m.m00, m.m10, m.m20);
                Math::Float3 cy(m.m01, m.m11, m.m21);
                Math::Float3 cz(m.m02, m.m12, m.m22);

                // Remove scale
                float sx = cx.GetLength();
                float sy = cy.GetLength();
                float sz = cz.GetLength();

                out_submodel.scale = { sx, sy, sz };

                if (sx == 0) sx = 1;
                if (sy == 0) sy = 1;
                if (sz == 0) sz = 1;

                Math::Float3 rx = cx / sx;
                Math::Float3 ry = cy / sy;
                Math::Float3 rz = cz / sz;

                float det = Math::Float3::Dot(Math::Float3::Cross(rx, ry), rz);
                if (det < 0.0f) 
                {
                    rx = rx * -1.0f;
                }

                float r00 = rx.x, r01 = ry.x, r02 = rz.x;
                float r10 = rx.y, r11 = ry.y, r12 = rz.y;
                float r20 = rx.z, r21 = ry.z, r22 = rz.z;

                float y = asinf(-Math::Clamp(r20, -1.0f, 1.0f));
                float x = atan2f(r21, r22);
                float z = atan2f(r10, r00);

                out_submodel.rotation.x = Math::RadiansToDegrees(x);
                out_submodel.rotation.y = Math::RadiansToDegrees(y);
                out_submodel.rotation.z = Math::RadiansToDegrees(z);
            }

            return out_submodel;
        }

        // ---------------------------------------------------------------------------
        //								    Fonts
        // ---------------------------------------------------------------------------

        bool LoadFont(const char* path, LoadedFont* out_font, uint32_t font_size)
        {
            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            RB_LOG(LOGTAG_MAIN, "Loading font: %s", final_path.c_str());

            LoadedImage* img = &out_font->fontAtlas;
            *img = {};
            img->loadedUsingStb = false;
            img->format         = RenderResourceFormat::R8_UNORM;

            FT_Library ft;
            if (FT_Init_FreeType(&ft))
            {
                RB_LOG_ERROR(LOGTAG_MAIN, " Could not init FreeType");
                return false;
            }
            out_font->fontLibrary = ft;

            auto file_handle = FileLoader::OpenFile(final_path.c_str(), OpenFileMode::kFileMode_Read | OpenFileMode::kFileMode_Binary);

            FileData file_data = file_handle->ReadFull();

            FT_Face face;
            if (FT_New_Memory_Face(ft, file_data.data, file_data.size, 0, &face))
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Failed to load font");
                return false;
            }
            out_font->fontFace = face;

            FT_Set_Pixel_Sizes(face, 0, font_size);

            // Calculate the width and height of the font atlas
            for (uint8_t c = 0; c < 128; c++)
            {
                FT_Load_Char(face, c, FT_LOAD_RENDER);
                img->width += face->glyph->bitmap.width;
                img->height = Math::Max(img->height, (int32_t)face->glyph->bitmap.rows);
            }

            img->dataSize = sizeof(uint8_t) * img->width * img->height;
            img->data = (uint8_t*)ALLOC_HEAP(img->dataSize);
            memset(img->data, 0, img->dataSize);

            // Just load the first 128 ASCII characters
            uint32_t offset = 0;
            for (uint8_t c = 0; c < 128; c++)
            {
                if (FT_Load_Char(face, c, FT_LOAD_RENDER))
                {
                    RB_LOG_ERROR(LOGTAG_MAIN, "Failed to load glyph: %c", c);
                    continue;
                }
                
                FT_GlyphSlot g = face->glyph;

                int pitch = g->bitmap.pitch;
                const uint8_t* src_buffer = g->bitmap.buffer;
                
                if (pitch < 0)
                {
                    // rows are stored bottom-up, reverse this
                    src_buffer += g->bitmap.rows * pitch;
                    pitch = -pitch;
                }
                
                for (uint32_t row = 0; row < g->bitmap.rows; ++row)
                {
                    // Write to the destination from bottom to top
                    uint32_t flipped_row = g->bitmap.rows - 1 - row;
                    memcpy((uint8_t*)img->data + flipped_row * img->width + offset,
                           src_buffer + row * pitch,
                           g->bitmap.width);
                }
                
                LoadedFont::Character character = {};
                character.imageUV       = Math::Float4((float)offset / img->width, 0.0f, (float)(offset + g->bitmap.width) / img->width, (float)g->bitmap.rows / img->height);
                character.size          = Math::Float2(g->bitmap.width, g->bitmap.rows);
                character.bearing       = Math::Float2(g->bitmap_left, g->bitmap_top);
                character.advance       = g->advance.x >> 6; // Go from 26.6 fixed point to regular int
                
                out_font->characters.emplace(c, character);
                
                offset += g->bitmap.width;
            }

            return true;
        }
    }
}