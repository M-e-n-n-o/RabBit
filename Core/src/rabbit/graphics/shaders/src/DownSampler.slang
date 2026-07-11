module DownSampler;
import Common;

#define GROUP_SIZE 16

ConstantBuffer<MipGeneratorCB> g_MipGeneratorCB;

groupshared float4 tile[GROUP_SIZE][GROUP_SIZE];

// TODO: Not completely sure if this is correct for sRGB color space
float4 Reduce2x2(float4 a, float4 b, float4 c, float4 d)
{
    return (a + b + c + d) * 0.25f;
}

struct Handles
{
    Texture2D<float4>.Handle        source;
    RWTexture2DArray<float4>.Handle target;
}

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void GenerateMips(uint3 group_id  : SV_GroupID, uint3 local_id  : SV_GroupThreadID, uniform Handles handles)
{
    uint2 local = local_id.xy;
    uint2 base = group_id.xy * GROUP_SIZE;

    // Load source mip
    uint2 srcPos = base + local;
    float4 value = 0;
    if (all(srcPos < g_MipGeneratorCB.size))
    {
        value = handles.source.Load(int3(srcPos, g_MipGeneratorCB.startMip));
    }

    tile[local.y][local.x] = value;

    GroupMemoryBarrierWithGroupSync();

    // Generate following mip levels
    for (uint i = 1; i <= g_MipGeneratorCB.mipCount; i++)
    {
        uint srcDim = GROUP_SIZE >> (i-1);
        uint dstDim = GROUP_SIZE >> i;

        // Only active threads participate
        if (local.x < dstDim && local.y < dstDim)
        {
            uint x = local.x * 2;
            uint y = local.y * 2;

            float4 a = tile[y + 0][x + 0];
            float4 b = tile[y + 0][x + 1];
            float4 c = tile[y + 1][x + 0];
            float4 d = tile[y + 1][x + 1];
            value = Reduce2x2(a, b, c, d);

            // Write into LDS for next iteration
            tile[local.y][local.x] = value;

            uint mip = g_MipGeneratorCB.startMip + i;
            uint2 outPos = (base / (1 << i)) + local;

            handles.target[uint3(outPos, mip)] = value;
        }

        GroupMemoryBarrierWithGroupSync();
    }
}