#pragma once

#include "RenderResource.h"

namespace RB::Graphics
{
    extern Texture2D* g_TexDefaultError;

    void InitResourceDefaults();
    void DeleteResourceDefaults();
}