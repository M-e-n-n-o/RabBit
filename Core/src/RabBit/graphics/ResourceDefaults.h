#pragma once

#include "RenderResource.h"

namespace RB::Graphics
{
    extern Texture2D* g_TexDefaultError;

    extern uint32_t g_TexDefaultErrorData[];

    void InitResourceDefaults();
    void DeleteResourceDefaults();
}