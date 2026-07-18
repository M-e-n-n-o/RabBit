#pragma once

#include "RenderResource.h"

namespace RB::Graphics
{
    extern Shared<Texture2D> g_TexDefaultError;
    extern Shared<Texture2D> g_TexDefaultWhite;
    extern Shared<Texture2D> g_TexDefaultBlack;

    extern Shared<VertexBuffer> g_FullscreenTriangle;

    void InitResourceDefaults();
    void DeleteResourceDefaults();
}