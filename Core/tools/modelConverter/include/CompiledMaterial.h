#pragma once

#include <cstdint>

#include "Magic.h"

namespace RB::ModelConverter
{
    inline constexpr uint32_t kMaterialMagic            = Tools::CreateMagic('R', 'B', 'M', 'T');
    inline constexpr uint32_t kMaterialVersion          = 1;
    inline constexpr uint32_t kMaxMaterialNameLength    = 32;
    inline constexpr uint32_t kMaxTextureNameLength     = 128;

    enum class BlendMode : uint8_t
    {
        Opaque,
        Blended,
    };

    struct CompiledMaterial
    {
        uint32_t    magic;
        uint32_t    version;
        char        name[kMaxMaterialNameLength];
        BlendMode   blendMode;
        uint8_t     alphaCutoutThreshold;
        uint8_t     pad0[2];

        // Texture file names (no directory), null terminated. Empty if the material has no such texture
        char        diffuseTexture[kMaxTextureNameLength];
        char        normalTexture[kMaxTextureNameLength];
    };
    static_assert(sizeof(CompiledMaterial) == 300);
}
