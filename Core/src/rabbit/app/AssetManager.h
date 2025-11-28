#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

#if defined(LoadImage)
#undef LoadImage
#endif

namespace RB
{
    struct LoadedImage;
    struct LoadedMesh;
    struct LoadedFont;

    namespace AssetManager
    {
        void Init(const char* asset_base_path);

        bool LoadImage8Bit(const char* path, LoadedImage* out_image, bool srgb);

        bool LoadMesh(const char* path, LoadedMesh* out_mesh);

        bool LoadFont(const char* path, LoadedFont* out_font, uint32_t font_size);
    }

    struct LoadedImage
    {
        void*                           data;
        uint32_t				        dataSize;
        Graphics::RenderResourceFormat	format;
        int32_t					        width;
        int32_t					        height;

        LoadedImage();
        ~LoadedImage();

    private:
        bool loadedUsingStb;

        friend bool AssetManager::LoadImage8Bit(const char*, LoadedImage*, bool);
        friend bool AssetManager::LoadFont(const char*, LoadedFont*, uint32_t);
    };

    struct LoadedMesh
    {
        struct Vertex
        {
            Math::Float3    normal;
            Math::Float2    uv;
        };

        struct Submodel
        {
            List<Math::Float3> positions;
            List<Vertex>       vertices;
            List<uint32_t>     indices;
            Math::Float3       position;
            Math::Float3       rotation;
            Math::Float3       minBounds;
            Math::Float3       maxBounds;
            uint32_t           albedoIndex;
        };

        List<std::string>   albedoTextures;
        List<Submodel>      models;
        void*               internalScene;

        LoadedMesh();
        ~LoadedMesh();
    };

    struct LoadedFont
    {
        struct Character
        {
            Math::Float4        imageUV;    // Texture UV coordinates for specific char in image
            Math::Float2        size;       // Size of glyph
            Math::Float2        bearing;    // Offset from baseline to left/top of glyph
            uint32_t            advance;    // Offset to advance to next glyph
        };

        Map<char, Character>    characters;
        LoadedImage             fontAtlas;

        LoadedFont();
        ~LoadedFont();

    private:
        void* fontLibrary;
        void* fontFace;

        friend bool AssetManager::LoadFont(const char*, LoadedFont*, uint32_t);
    };
}