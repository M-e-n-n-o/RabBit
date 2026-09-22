#pragma once

#include <cstdint>

#include "Magic.h"

namespace RB::ModelConverter
{
    inline constexpr uint32_t ValidMagic        = Tools::CreateMagic('R', 'B', 'M', 'D');
    inline constexpr uint32_t kCurrentVersion   = 1;
    inline constexpr uint32_t kInvalidIndex     = 0xFFFFFFFFu;
    inline constexpr uint32_t kMaxNameLength    = 32;
    inline constexpr uint32_t kMaxTextureLength = 128;
    inline constexpr uint32_t kMaxInfluences    = 4;  // Bone influences per vertex

    struct PackedFloat2 { float x, y; };
    struct PackedFloat3 { float x, y, z; };
    struct PackedFloat4 { float x, y, z, w; };

    enum SubmodelFlags : uint32_t
    {
        kSubmodel_Skinned = 1 << 0  // Has skinVertices/skinBones. Once you do skinning, the bone matrices already
                                    // contain the world transform, so the mesh node's own transform must not be applied on top.
    };

    struct CompiledModelHeader
    {
        uint32_t    magic;
        uint32_t    version;
        char        name[kMaxNameLength];

        uint32_t    materialCount;
        uint32_t    submodelCount;
        uint32_t    nodeCount;
        uint32_t    animationCount;

        uint64_t    materialsOffset;    // CompiledMaterial[materialCount]
        uint64_t    submodelsOffset;    // CompiledSubmodel[submodelCount]
        uint64_t    nodesOffset;        // CompiledNode[nodeCount]
        uint64_t    animationsOffset;   // CompiledAnimation[animationCount]
        uint64_t    fileSize;
    };
    static_assert(sizeof(CompiledModelHeader) == 96);

    struct CompiledMaterial
    {
        char        diffuseTexture[kMaxTextureLength];  // Texture file name (no directory), null terminated
    };

    struct CompiledVertex
    {
        PackedFloat3    normal;
        PackedFloat2    uv;
    };
    static_assert(sizeof(CompiledVertex) == 20);

    struct CompiledSkinVertex
    {
        uint16_t    bones[kMaxInfluences];      // Index into this submodel's CompiledSkinBone array
        float       weights[kMaxInfluences];    // Normalized, strongest first. Unused slots are 0
    };
    static_assert(sizeof(CompiledSkinVertex) == 24);

    struct CompiledSkinBone
    {
        uint32_t    nodeIndex;              // Index into the node array
        float       geometryToBone[16];     // "Inverse bind" matrix, row-major
    };
    static_assert(sizeof(CompiledSkinBone) == 68);

    struct CompiledSubmodel
    {
        uint32_t        nodeIndex;          // Node this mesh is attached to
        uint32_t        materialIndex;      // Index into materials, kInvalidIndex if none
        uint32_t        vertexCount;
        uint32_t        indexCount;
        uint32_t        boneCount;
        uint32_t        flags;              // SubmodelFlags

        PackedFloat3    minBounds;          // In the local space of the node
        PackedFloat3    maxBounds;

        uint64_t        positionsOffset;    // PackedFloat3[vertexCount]
        uint64_t        verticesOffset;     // CompiledVertex[vertexCount]
        uint64_t        indicesOffset;      // uint32_t[indexCount]
        uint64_t        skinVerticesOffset; // CompiledSkinVertex[vertexCount]  (skinned only)
        uint64_t        skinBonesOffset;    // CompiledSkinBone[boneCount]      (skinned only)
    };
    static_assert(sizeof(CompiledSubmodel) == 88);

    struct CompiledNode
    {
        char            name[kMaxNameLength];
        int32_t         parent;             // -1 for root. Parents always come before their children
        PackedFloat3    translation;        // Bind pose local transform
        PackedFloat4    rotation;           // Quaternion
        PackedFloat3    scale;
    };
    static_assert(sizeof(CompiledNode) == 76);

    struct CompiledKey
    {
        float           time;               // Seconds since the start of the animation
        PackedFloat3    value;              // Translation or scale depending on the track
    };
    static_assert(sizeof(CompiledKey) == 16);

    struct CompiledRotationKey
    {
        float           time;               // Seconds since the start of the animation
        PackedFloat4    value;              // Quaternion. Consecutive keys are in the same hemisphere (dot >= 0), so nlerp/slerp takes the short way
    };
    static_assert(sizeof(CompiledRotationKey) == 20);

    struct CompiledChannel
    {
        uint32_t        nodeIndex;
        uint32_t        translationKeyCount;
        uint32_t        rotationKeyCount;
        uint32_t        scaleKeyCount;
        uint64_t        translationKeysOffset;  // CompiledKey[translationKeyCount]
        uint64_t        rotationKeysOffset;     // CompiledRotationKey[rotationKeyCount]
        uint64_t        scaleKeysOffset;        // CompiledKey[scaleKeyCount]
    };
    static_assert(sizeof(CompiledChannel) == 40);

    struct CompiledAnimation
    {
        char            name[kMaxNameLength];
        float           duration;           // Seconds
        uint32_t        channelCount;       // Only nodes that are actually animated get a channel
        uint64_t        channelsOffset;     // CompiledChannel[channelCount]
    };
    static_assert(sizeof(CompiledAnimation) == 48);
}
