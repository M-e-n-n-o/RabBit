#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

#if defined(LoadImage)
#undef LoadImage
#endif

namespace RB
{
    struct LoadedImage;
    struct LoadedModel;
    struct LoadedFont;

    namespace AssetLoader
    {
        void Init(const char* asset_base_path);

        bool LoadConvertedTexture(const char* path, LoadedImage* out_image);
        bool LoadTexture8Bit(const char* path, LoadedImage* out_image, bool srgb); // Should ideally not be used in a final build

        bool LoadConvertedModel(const char* path, LoadedModel* out_model);

        bool LoadFont(const char* path, LoadedFont* out_font, uint32_t font_size);
    }

    struct LoadedImage
    {
        char                            name[30];
        void*                           data;
        uint32_t                        dataSize;
        Graphics::RenderResourceFormat  format;
        int32_t                         width;
        int32_t                         height;
        uint32_t                        mipCount;

        LoadedImage();
        ~LoadedImage();

    private:
        bool loadedUsingStb;

        friend bool AssetLoader::LoadConvertedTexture(const char*, LoadedImage*);
        friend bool AssetLoader::LoadTexture8Bit(const char*, LoadedImage*, bool);
        friend bool AssetLoader::LoadFont(const char*, LoadedFont*, uint32_t);
    };

    struct LoadedModel
    {
        struct Vertex
        {
            Math::Float3    normal;
            Math::Float2    uv;
        };

        struct SkinVertex
        {
            uint16_t    bones[4];       // Index into Submodel::skinBones
            float       weights[4];     // Normalized, strongest first. Unused slots are 0
        };

        struct SkinBone
        {
            uint32_t       nodeIndex;          // Index into LoadedModel::nodes
            Math::Float4x4 geometryToBone;     // "Inverse bind" matrix
        };

        struct Submodel
        {
            List<Math::Float3> positions;
            List<Vertex>       vertices;
            List<uint32_t>     indices;
            Math::Float3       minBounds;           // In the local space of the node
            Math::Float3       maxBounds;
            uint32_t           diffuseTexIndex;     // Index into diffuseColorTextures (0xFFFFFFFF = none)
            uint32_t           nodeIndex;           // Node this mesh is attached to. The vertices are in that node's local space
            bool               skinned;             // When skinned the bone matrices contain the world transform, so the node's own transform must not be applied on top
            List<SkinVertex>   skinVertices;
            List<SkinBone>     skinBones;
        };

        // Node hierarchy / skeleton
        struct Node
        {
            std::string         name;
            int32_t             parent;         // -1 for root. Parents always come before their children
            Math::Float3        translation;    // Bind pose local transform
            Math::Quaternion    rotation;
            Math::Float3        scale;
        };

        struct Float3Key { float time; Math::Float3 value; };
        struct QuatKey   { float time; Math::Quaternion value; };

        struct AnimationChannel
        {
            uint32_t            nodeIndex;
            List<Float3Key>     translation;    // Empty = not animated, use the bind pose
            List<QuatKey>       rotation;
            List<Float3Key>     scale;
        };

        struct Animation
        {
            std::string             name;
            float                   duration;   // Seconds
            List<AnimationChannel>  channels;   // Only nodes that are animated
        };

        List<std::string>   diffuseColorTextures;
        List<Submodel>      models;
        List<Node>          nodes;
        List<Animation>     animations;
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

        friend bool AssetLoader::LoadFont(const char*, LoadedFont*, uint32_t);
    };
}