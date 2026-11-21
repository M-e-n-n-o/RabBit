#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CompiledMesh.h"
#include "CompiledMaterial.h"

// Format neutral scene that every importer fills in and every writer reads from.
// Nothing in here may depend on a specific file format library (ufbx etc.)
namespace RB::ModelConverter
{
    struct ImportedSubmesh
    {
        uint32_t                        nodeIndex       = kInvalidIndex;
        uint32_t                        materialIndex   = kInvalidIndex;    // Index into ImportedScene::materials
        uint32_t                        flags           = 0;                // SubmeshFlags
        PackedFloat3                    minBounds       = {};
        PackedFloat3                    maxBounds       = {};
        std::vector<PackedFloat3>       positions;
        std::vector<CompiledVertex>     vertices;
        std::vector<uint32_t>           indices;
        std::vector<CompiledSkinVertex> skinVertices;
        std::vector<CompiledSkinBone>   skinBones;
    };

    struct ImportedChannel
    {
        uint32_t                            nodeIndex = 0;
        std::vector<CompiledKey>            translation;
        std::vector<CompiledRotationKey>    rotation;
        std::vector<CompiledKey>            scale;
    };

    struct ImportedAnimation
    {
        std::string                     name;
        float                           duration = 0.0f;
        std::vector<ImportedChannel>    channels;
    };

    struct ImportedMaterial
    {
        std::string     name;                               // As found in the source file, may be empty
        BlendMode       blendMode = BlendMode::Opaque;
        // File names only (no directory) with the source extension. Empty if there is no texture
        std::string     diffuseTexture;
        std::string     normalTexture;
    };

    struct ImportedScene
    {
        std::vector<CompiledNode>       nodes;              // Parents always come before their children
        std::vector<ImportedSubmesh>    submeshes;
        std::vector<ImportedAnimation>  animations;
        std::vector<ImportedMaterial>   materials;          // Empty if the source format has no materials
    };

    struct ImportOptions
    {
        bool    importAnimations    = true;
        double  animationSampleRate = 30.0;
    };
}
