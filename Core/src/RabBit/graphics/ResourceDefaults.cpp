#include "RabBitCommon.h"
#include "ResourceDefaults.h"

namespace RB::Graphics
{
	// ---------------------------------------------------------------------------
	//							Texture Defines
	// ---------------------------------------------------------------------------

	Texture2D* g_TexDefaultError = nullptr;
	
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

	// ---------------------------------------------------------------------------
	//							Initialization
	// ---------------------------------------------------------------------------

	void InitResourceDefaults()
	{
		g_TexDefaultError = Texture2D::Create("Default error texture 2D", g_TexDefaultErrorData, sizeof(g_TexDefaultErrorData), RenderResourceFormat::R8G8B8A8_UNORM, 4, 4, false, false, false);
	}
	
	void DeleteResourceDefaults()
	{
		delete g_TexDefaultError;
	}
}