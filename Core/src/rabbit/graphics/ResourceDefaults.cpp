#include "RabBitCommon.h"
#include "ResourceDefaults.h"

namespace RB::Graphics
{
    // ---------------------------------------------------------------------------
    //							Texture Defines
    // ---------------------------------------------------------------------------

    Shared<Texture2D> g_TexDefaultError = nullptr;
    Shared<Texture2D> g_TexDefaultWhite = nullptr;
    Shared<Texture2D> g_TexDefaultBlack = nullptr;

    // ---------------------------------------------------------------------------
    //							 Texture Data
    // ---------------------------------------------------------------------------

    #define RGBA8(r,g,b,a)  ( (uint32_t)( ((a)<<24) | ((b)<<16) | ((g)<<8) | (r) ) )

    uint32_t g_TexDefaultErrorData[] =
    {
        RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF),
        RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF),
        RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF),
        RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF), RGBA8(0xFF, 0x00, 0x00, 0xFF),
    };

    uint32_t g_TexDefaultWhiteData[] =
    {
        RGBA8(0xFF, 0xFF, 0xFF, 0xFF)
    };

    uint32_t g_TexDefaultBlackData[] =
    {
        RGBA8(0x00, 0x00, 0x00, 0xFF)
    };

    // ---------------------------------------------------------------------------
    //							Initialization
    // ---------------------------------------------------------------------------

    void InitResourceDefaults()
    {
        g_TexDefaultError = Texture2D::Create("Default error texture 2D", g_TexDefaultErrorData, sizeof(g_TexDefaultErrorData), RenderResourceFormat::R8G8B8A8_UNORM, 4, 4, false, false);
        g_TexDefaultWhite = Texture2D::Create("Default white texture 2D", g_TexDefaultWhiteData, sizeof(g_TexDefaultWhiteData), RenderResourceFormat::R8G8B8A8_UNORM, 1, 1, false, false);
        g_TexDefaultBlack = Texture2D::Create("Default black texture 2D", g_TexDefaultBlackData, sizeof(g_TexDefaultBlackData), RenderResourceFormat::R8G8B8A8_UNORM, 1, 1, false, false);
    }

    void DeleteResourceDefaults()
    {
        g_TexDefaultError.reset();
        g_TexDefaultWhite.reset();
        g_TexDefaultBlack.reset();
    }
}