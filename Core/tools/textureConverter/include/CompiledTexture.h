#pragma once

#include <cstdint>

#include "Magic.h"

namespace RB::TextureConverter
{
    enum Format : uint8_t
    {
        kFormat_Invalid,

        // Raw formats
        kFormat_R8,
        kFormat_RGBA8,
        kFormat_RGBA8_SRGB,
        
        // Compressed formats
        kFormat_BC1,        // RGB565 (+ optional 1-bit Alpha)  (0.5 Bytes/px)      (e.g. Simple textures with max 1 bit transparency)
        kFormat_BC1_SRGB,
        kFormat_BC3,        // RGB565+A8                        (1 Byte/px)         (e.g. Textures with multiple levels of transparency)
        kFormat_BC3_SRGB,
        kFormat_BC4,        // R8                               (0.5 Bytes/px)      (e.g. Height, metallic, roughness maps)
        kFormat_BC5,        // RG88                             (1 Byte/px)         (e.g. Normal maps)

        // TODO: Implement BC6 & BC7

        kFormat_Count
    };

    inline constexpr uint32_t ValidMagic = Tools::CreateMagic('R', 'B', 'T', 'X');
    inline constexpr int MaxMips = UINT8_MAX;

    struct CompiledTextureHeader
    {
        uint32_t                        magic;
        char                            name[30];
        uint8_t                         format;
        uint8_t                         targetMips;
        uint32_t                        width;
        uint32_t                        height;
        uint64_t                        dataSize;
    };
}