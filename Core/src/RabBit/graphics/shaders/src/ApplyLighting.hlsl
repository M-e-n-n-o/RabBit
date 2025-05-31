#include "../shared/Common.h"
#include "Lighting.h"
#include "GBuffer.h"

[numthreads(8, 8, 1)]
void CS_ApplyLightingDeferred(uint2 screen_coord : SV_DispatchThreadID)
{
    GBufferTexIndices indices;
    indices.gbuf0 = 0;
    indices.gbuf1 = 1;

    GBuffer gbuf = SampleGBuffer(indices, screen_coord);

    GetBlinnPhongDiffSpec();
}