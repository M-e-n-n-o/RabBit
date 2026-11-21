#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

namespace RB
{
    struct LoadedTexture;
    struct LoadedMesh;
    struct LoadedMaterial;
    struct LoadedModel;
    struct LoadedFont;

    namespace AssetLoader
    {
        void Init(const char* asset_base_path);

        bool LoadConvertedTexture(const char* path, LoadedTexture* out_tex);       // .rbtx
        bool LoadTexture8Bit(const char* path, LoadedTexture* out_tex, bool srgb); // Should ideally not be used in a final build

        bool LoadConvertedMesh(const char* path, LoadedMesh* out_mesh);             // .rbms
        bool LoadConvertedMaterial(const char* path, LoadedMaterial* out_material); // .rbmt

        // .rbmd: only parses the model file itself, the referenced mesh and materials still need to be loaded by the caller
        bool LoadConvertedModel(const char* path, LoadedModel* out_model);

        bool LoadFont(const char* path, LoadedFont* out_font, uint32_t font_size);
    }

    struct LoadedTexture
    {
        char                            name[30];
        void*                           data;
        uint32_t                        dataSize;
        Graphics::RenderResourceFormat  format;
        int32_t                         width;
        int32_t                         height;
        uint32_t                        mipCount;

        LoadedTexture();
        ~LoadedTexture();

    private:
        bool loadedUsingStb;

        friend bool AssetLoader::LoadConvertedTexture(const char*, LoadedTexture*);
        friend bool AssetLoader::LoadTexture8Bit(const char*, LoadedTexture*, bool);
        friend bool AssetLoader::LoadFont(const char*, LoadedFont*, uint32_t);
    };

    struct LoadedMesh
    {
        struct Vertex
        {
            Math::Float3    normal;
            Math::Float2    uv;
        };

        struct SkinVertex
        {
            uint16_t    bones[4];       // Index into Submesh::skinBones
            float       weights[4];     // Normalized, strongest first. Unused slots are 0
        };

        struct SkinBone
        {
            uint32_t       nodeIndex;
            Math::Float4x4 geometryToBone;     // "Inverse bind" matrix
        };

        struct Submesh
        {
            List<Math::Float3> positions;           // Are in local space of the Submesh
            List<Vertex>       vertices;
            List<uint32_t>     indices;
            Math::Float3       minBounds;           // In the local space of the node
            Math::Float3       maxBounds;
            uint32_t           nodeIndex;
            bool               isSkinned;
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

        // Entire animation for the specified node
        struct AnimationChannel
        {
            uint32_t            nodeIndex;
            List<Float3Key>     translations;    // Empty = not animated, use the bind pose
            List<QuatKey>       rotations;
            List<Float3Key>     scales;
        };

        struct Animation
        {
            std::string             name;
            float                   duration;   // Seconds
            List<AnimationChannel>  channels;
        };

        List<Submesh>       submeshes;
        List<Node>          nodes;
        List<Animation>     animations;
    };

    struct LoadedMaterial
    {
        enum class BlendMode : uint8_t
        {
            Opaque,
            Blended,
            Cutout
        };

        std::string     name;
        BlendMode       blendMode;
        std::string     diffuseTexture;     // Texture file names (no directory), empty if not set
        std::string     normalTexture;
    };

    struct LoadedModel
    {
        static constexpr uint32_t kNoMaterial = 0xFFFFFFFFu;

        std::string         name;

        // Paths are already resolved relative to the model file, so they can be passed straight to the other loaders
        std::string         meshPath;
        List<std::string>   materialPaths;
        List<uint32_t>      submeshMaterials;   // One per mesh submesh, index into materialPaths or kNoMaterial
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
        LoadedTexture           fontAtlas;

        LoadedFont();
        ~LoadedFont();

    private:
        void* fontLibrary;
        void* fontFace;

        friend bool AssetLoader::LoadFont(const char*, LoadedFont*, uint32_t);
    };
}