#pragma once

#include <cstdint>

#include "Magic.h"

namespace RB::ModelConverter
{
    inline constexpr uint32_t kModelMagic           = Tools::CreateMagic('R', 'B', 'M', 'D');
    inline constexpr uint32_t kModelVersion         = 1;
    inline constexpr uint32_t kMaxModelNameLength   = 32;
    inline constexpr uint32_t kMaxFileNameLength    = 128;
    inline constexpr uint32_t kNoMaterial           = 0xFFFFFFFFu;

    // A model only references other files. File names have no directory and are relative to the model file
    struct CompiledModelHeader
    {
        uint32_t    magic;
        uint32_t    version;
        char        name[kMaxModelNameLength];
        char        meshFile[kMaxFileNameLength];   // .rbms

        uint32_t    materialCount;
        uint32_t    submeshCount;                   // Matches the submesh count of the mesh file

        uint64_t    materialsOffset;                // CompiledModelMaterial[materialCount]
        uint64_t    submeshMaterialsOffset;         // uint32_t[submeshCount], index into the materials or kNoMaterial
        uint64_t    fileSize;
    };
    static_assert(sizeof(CompiledModelHeader) == 200);

    struct CompiledModelMaterial
    {
        char        file[kMaxFileNameLength];       // .rbmt
    };
    static_assert(sizeof(CompiledModelMaterial) == 128);
}
