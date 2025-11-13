#pragma once

#include "RenderResource.h"

namespace RB::Graphics
{
    extern Shared<Texture2D> g_TexDefaultError;

    void InitResourceDefaults();
    void DeleteResourceDefaults();
}