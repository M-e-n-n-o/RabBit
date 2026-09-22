#include "RabBitCommon.h"
#include "AssetLoader.h"
#include "utils/File.h"

#include <filesystem>

#include <CompiledTexture.h>
#include <CompiledModel.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

    namespace AssetLoader
    {
        static const char* g_AssetPath = nullptr;

        void Init(const char* asset_path)
        {
            g_AssetPath = asset_path;
        }

        // ---------------------------------------------------------------------------
        //                                  Images
        // ---------------------------------------------------------------------------

        bool LoadConvertedTexture(const char* path, LoadedImage* out_image)
        {
            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            RB_LOG(LOGTAG_MAIN, "Loading converted texture file: %s", final_path.c_str());

            auto file_handle = FileLoader::OpenFile(final_path.c_str(), OpenFileMode::kFileMode_Read | OpenFileMode::kFileMode_Binary);

            if (!file_handle->IsValid())
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Failed to load texture file \"%s\" from disk", final_path.c_str());
                return false;
            }

            FileData data = file_handle->ReadFull();

            if (data.size < sizeof(TextureConverter::CompiledTextureHeader))
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Texture file is not a valid converted texture (too small)");
                return false;
            }

            TextureConverter::CompiledTextureHeader* header = (TextureConverter::CompiledTextureHeader*)data.data;
            if (header->magic != TextureConverter::ValidMagic)
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Texture file is not a valid converted texture, incorrect magic: %d", header->magic);
                return false;
            }

            if (header->version != ModelConverter::kCurrentVersion)
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Texture file has version %d but version %d is expected, convert it again", header->version, ModelConverter::kCurrentVersion);
                return false;
            }

            out_image->width            = header->width;
            out_image->height           = header->height;
            out_image->dataSize         = header->dataSize;
            out_image->mipCount         = header->mipCount;
            out_image->loadedUsingStb   = false;
            switch (header->format)
            {
            case TextureConverter::kFormat_R8:          out_image->format = RenderResourceFormat::R8_UNORM; break;
            case TextureConverter::kFormat_RGBA8:       out_image->format = RenderResourceFormat::RGBA8_UNORM; break;
            case TextureConverter::kFormat_RGBA8_SRGB:  out_image->format = RenderResourceFormat::RGBA8_SRGB; break;
            case TextureConverter::kFormat_BC1:         out_image->format = RenderResourceFormat::BC1_UNORM; break;
            case TextureConverter::kFormat_BC1_SRGB:    out_image->format = RenderResourceFormat::BC1_SRGB; break;
            case TextureConverter::kFormat_BC3:         out_image->format = RenderResourceFormat::BC3_UNORM; break;
            case TextureConverter::kFormat_BC3_SRGB:    out_image->format = RenderResourceFormat::BC3_SRGB; break;
            case TextureConverter::kFormat_BC4:         out_image->format = RenderResourceFormat::BC4_UNORM; break;
            case TextureConverter::kFormat_BC5:         out_image->format = RenderResourceFormat::BC5_UNORM; break;
            default:
                RB_LOG_ERROR(LOGTAG_MAIN, "Did not recognize loaded texture format from file: %s", final_path.c_str());
                return false;
            }

            memcpy(out_image->name, header->name, _countof(out_image->name));

            out_image->data = ALLOC_HEAP(out_image->dataSize);
            memcpy(out_image->data, data.data + sizeof(TextureConverter::CompiledTextureHeader), out_image->dataSize);

            RB_LOG(LOGTAG_MAIN, "Loaded converted texture: %s", out_image->name);

            return true;
        }

        bool LoadTexture8Bit(const char* path, LoadedImage* out_image, bool srgb)
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
                actual_channels = 4; // We don't support 3 channel textures

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
                out_image->format = srgb ? RenderResourceFormat::RGBA8_SRGB : RenderResourceFormat::RGBA8_UNORM;
                break;
            case 0:
            case 2:
            case 3:
            default:
                RB_LOG_ERROR(LOGTAG_MAIN, "This many channels for an 8 bit image is not supported");
                out_image->format = RenderResourceFormat::Unkown;
                break;
            }

            out_image->mipCount = 1;
            out_image->dataSize = GetElementSizeFromFormat(out_image->format) * out_image->width * out_image->height;

            std::filesystem::path file_path(path);
            std::string file_path_name = file_path.stem().string();
            memset(out_image->name, 0, _countof(out_image->name));
            memcpy(out_image->name, file_path_name.data(), std::min(_countof(out_image->name), file_path_name.size()));

            return true;
        }

        // ---------------------------------------------------------------------------
        //                                  Models
        // ---------------------------------------------------------------------------

        template<typename T>
        static bool GetFileArray(const uint8_t* base, uint64_t file_size, uint64_t offset, uint64_t count, const T*& out)
        {
            out = nullptr;

            if (count == 0)
                return true;

            if (offset == 0 || offset > file_size)
                return false;

            if (count > (file_size - offset) / sizeof(T))
                return false;

            out = (const T*)(base + offset);
            return true;
        }

        static Math::Float3 ToFloat3(const ModelConverter::PackedFloat3& v)
        {
            return Math::Float3(v.x, v.y, v.z);
        }

        static Math::Quaternion ToQuaternion(const ModelConverter::PackedFloat4& q)
        {
            Math::Quaternion result = {};
            result.x = q.x;
            result.y = q.y;
            result.z = q.z;
            result.w = q.w;
            return result;
        }

        bool LoadConvertedModel(const char* path, LoadedModel* out_model)
        {
            // TODO:
            // - Store more texture types (normals, roughness, etc.)
            // - Do model loading on a different thread? (+ use thread pool for each submodel)
            //      - You can then choose the behaviour when its not yet loaded. Need to block until loaded or just skip rendering until loaded?

            // The bulk vertex data is copied straight from the file, so the layouts have to match
            static_assert(sizeof(Math::Float3)              == sizeof(ModelConverter::PackedFloat3), "Math::Float3 does not match the file layout, copy per element instead");
            static_assert(sizeof(LoadedModel::Vertex)       == sizeof(ModelConverter::CompiledVertex), "LoadedModel::Vertex does not match the file layout, copy per element instead");
            static_assert(sizeof(LoadedModel::SkinVertex)   == sizeof(ModelConverter::CompiledSkinVertex), "LoadedModel::SkinVertex does not match the file layout");
            static_assert(sizeof(Math::Float4x4)            == sizeof(float) * 16, "Math::Float4x4 does not match the file layout");

            std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

            RB_LOG(LOGTAG_MAIN, "Loading converted model file: %s", final_path.c_str());

            auto file_handle = FileLoader::OpenFile(final_path.c_str(), OpenFileMode::kFileMode_Read | OpenFileMode::kFileMode_Binary);

            if (!file_handle->IsValid())
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Failed to load model file \"%s\" from disk", final_path.c_str());
                return false;
            }

            FileData data = file_handle->ReadFull();
            const uint8_t* base = (const uint8_t*)data.data;
            const uint64_t file_size = data.size;

            if (file_size < sizeof(ModelConverter::CompiledModelHeader))
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is not a valid converted model (too small)", final_path.c_str());
                return false;
            }

            ModelConverter::CompiledModelHeader header;
            memcpy(&header, base, sizeof(header));

            if (header.magic != ModelConverter::ValidMagic)
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is not a valid converted model, incorrect magic: %d", final_path.c_str(), header.magic);
                return false;
            }

            if (header.version != ModelConverter::kCurrentVersion)
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" has version %d but version %d is expected, convert it again", final_path.c_str(), header.version, ModelConverter::kCurrentVersion);
                return false;
            }

            if (header.fileSize != file_size)
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is truncated or corrupt", final_path.c_str());
                return false;
            }

            const ModelConverter::CompiledMaterial*  materials  = nullptr;
            const ModelConverter::CompiledSubmodel*  submodels  = nullptr;
            const ModelConverter::CompiledNode*      nodes      = nullptr;
            const ModelConverter::CompiledAnimation* animations = nullptr;

            if (!GetFileArray(base, file_size, header.materialsOffset,  header.materialCount,  materials)  ||
                !GetFileArray(base, file_size, header.submodelsOffset,  header.submodelCount,  submodels)  ||
                !GetFileArray(base, file_size, header.nodesOffset,      header.nodeCount,      nodes)      ||
                !GetFileArray(base, file_size, header.animationsOffset, header.animationCount, animations))
            {
                RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is corrupt (tables are out of bounds)", final_path.c_str());
                return false;
            }

            // Materials
            for (uint32_t i = 0; i < header.materialCount; i++)
            {
                const char* name = materials[i].diffuseTexture;
                out_model->diffuseColorTextures.push_back(std::string(name, strnlen(name, ModelConverter::kMaxTextureLength)));
            }

            // Nodes
            out_model->nodes.resize(header.nodeCount);
            for (uint32_t i = 0; i < header.nodeCount; i++)
            {
                const ModelConverter::CompiledNode& src = nodes[i];
                LoadedModel::Node& dst = out_model->nodes[i];

                if (src.parent >= (int32_t)i)
                {
                    RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is corrupt (node %d has an invalid parent)", final_path.c_str(), i);
                    return false;
                }

                dst.name        = std::string(src.name, strnlen(src.name, ModelConverter::kMaxNameLength));
                dst.parent      = src.parent;
                dst.translation = ToFloat3(src.translation);
                dst.rotation    = ToQuaternion(src.rotation);
                dst.scale       = ToFloat3(src.scale);
            }

            // Submodels
            out_model->models.resize(header.submodelCount);
            for (uint32_t i = 0; i < header.submodelCount; i++)
            {
                const ModelConverter::CompiledSubmodel& src = submodels[i];
                LoadedModel::Submodel& dst = out_model->models[i];

                const ModelConverter::PackedFloat3*         positions       = nullptr;
                const ModelConverter::CompiledVertex*       vertices        = nullptr;
                const uint32_t*                             indices         = nullptr;
                const ModelConverter::CompiledSkinVertex*   skin_vertices   = nullptr;
                const ModelConverter::CompiledSkinBone*     skin_bones      = nullptr;

                const bool skinned = (src.flags & ModelConverter::kSubmodel_Skinned) != 0;

                if (src.nodeIndex >= header.nodeCount ||
                    !GetFileArray(base, file_size, src.positionsOffset, src.vertexCount, positions) ||
                    !GetFileArray(base, file_size, src.verticesOffset,  src.vertexCount, vertices)  ||
                    !GetFileArray(base, file_size, src.indicesOffset,   src.indexCount,  indices)   ||
                    (skinned && (!GetFileArray(base, file_size, src.skinVerticesOffset, src.vertexCount, skin_vertices) ||
                                 !GetFileArray(base, file_size, src.skinBonesOffset,    src.boneCount,   skin_bones))))
                {
                    RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is corrupt (submodel %d is out of bounds)", final_path.c_str(), i);
                    return false;
                }

                dst.minBounds       = ToFloat3(src.minBounds);
                dst.maxBounds       = ToFloat3(src.maxBounds);
                dst.diffuseTexIndex = src.materialIndex;
                dst.nodeIndex       = src.nodeIndex;
                dst.skinned         = skinned;

                dst.positions.resize(src.vertexCount);
                dst.vertices.resize(src.vertexCount);
                dst.indices.resize(src.indexCount);
                if (src.vertexCount > 0)
                {
                    memcpy(dst.positions.data(), positions, sizeof(ModelConverter::PackedFloat3) * src.vertexCount);
                    memcpy(dst.vertices.data(),  vertices,  sizeof(ModelConverter::CompiledVertex) * src.vertexCount);
                }
                if (src.indexCount > 0)
                    memcpy(dst.indices.data(), indices, sizeof(uint32_t) * src.indexCount);

                if (skinned)
                {
                    dst.skinVertices.resize(src.vertexCount);
                    if (src.vertexCount > 0)
                        memcpy(dst.skinVertices.data(), skin_vertices, sizeof(ModelConverter::CompiledSkinVertex) * src.vertexCount);

                    dst.skinBones.resize(src.boneCount);
                    for (uint32_t b = 0; b < src.boneCount; b++)
                    {
                        if (skin_bones[b].nodeIndex >= header.nodeCount)
                        {
                            RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is corrupt (submodel %d has a bone without a node)", final_path.c_str(), i);
                            return false;
                        }

                        dst.skinBones[b].nodeIndex = skin_bones[b].nodeIndex;
                        memcpy(&dst.skinBones[b].geometryToBone, skin_bones[b].geometryToBone, sizeof(float) * 16);
                    }
                }
            }

            // Animations
            out_model->animations.resize(header.animationCount);
            for (uint32_t i = 0; i < header.animationCount; i++)
            {
                const ModelConverter::CompiledAnimation& src = animations[i];
                LoadedModel::Animation& dst = out_model->animations[i];

                const ModelConverter::CompiledChannel* channels = nullptr;
                if (!GetFileArray(base, file_size, src.channelsOffset, src.channelCount, channels))
                {
                    RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is corrupt (animation %d is out of bounds)", final_path.c_str(), i);
                    return false;
                }

                dst.name     = std::string(src.name, strnlen(src.name, ModelConverter::kMaxNameLength));
                dst.duration = src.duration;
                dst.channels.resize(src.channelCount);

                for (uint32_t c = 0; c < src.channelCount; c++)
                {
                    const ModelConverter::CompiledChannel& src_ch = channels[c];
                    LoadedModel::AnimationChannel& dst_ch = dst.channels[c];

                    const ModelConverter::CompiledKey*          translation = nullptr;
                    const ModelConverter::CompiledRotationKey*  rotation    = nullptr;
                    const ModelConverter::CompiledKey*          scale       = nullptr;

                    if (src_ch.nodeIndex >= header.nodeCount ||
                        !GetFileArray(base, file_size, src_ch.translationKeysOffset, src_ch.translationKeyCount, translation) ||
                        !GetFileArray(base, file_size, src_ch.rotationKeysOffset,    src_ch.rotationKeyCount,    rotation)    ||
                        !GetFileArray(base, file_size, src_ch.scaleKeysOffset,       src_ch.scaleKeyCount,       scale))
                    {
                        RB_LOG_ERROR(LOGTAG_MAIN, "Model file \"%s\" is corrupt (animation %d has an invalid channel)", final_path.c_str(), i);
                        return false;
                    }

                    dst_ch.nodeIndex = src_ch.nodeIndex;

                    dst_ch.translation.resize(src_ch.translationKeyCount);
                    for (uint32_t k = 0; k < src_ch.translationKeyCount; k++)
                        dst_ch.translation[k] = { translation[k].time, ToFloat3(translation[k].value) };

                    dst_ch.rotation.resize(src_ch.rotationKeyCount);
                    for (uint32_t k = 0; k < src_ch.rotationKeyCount; k++)
                        dst_ch.rotation[k] = { rotation[k].time, ToQuaternion(rotation[k].value) };

                    dst_ch.scale.resize(src_ch.scaleKeyCount);
                    for (uint32_t k = 0; k < src_ch.scaleKeyCount; k++)
                        dst_ch.scale[k] = { scale[k].time, ToFloat3(scale[k].value) };
                }
            }

            RB_LOG(LOGTAG_MAIN, "Loaded converted model: %s (%d submodels, %d nodes, %d animations)", header.name, header.submodelCount, header.nodeCount, header.animationCount);

            return true;
        }

        // ---------------------------------------------------------------------------
        //                                  Fonts
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