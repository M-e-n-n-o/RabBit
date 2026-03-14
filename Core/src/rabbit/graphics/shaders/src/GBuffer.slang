module GBuffer;
import Common;

public struct GBuffer
{
    public float4 diffColor;
    public float3 normal;
    public float  depth;
};

public struct GBufferEncoded
{
    public float4 gbuf0;
    public float4 gbuf1;
};

public struct GBufferTexHandles
{
    Texture2D<float4>.Handle gbuf0;
    Texture2D<float4>.Handle gbuf1;
};

public GBufferEncoded EncodeGBuffer(GBuffer gbuf)
{
    GBufferEncoded enc;
    enc.gbuf0 = gbuf.diffColor;
    enc.gbuf1 = float4(gbuf.normal.rgb, gbuf.depth);
    return enc;
}

public GBuffer DecodeGBuffer(GBufferEncoded enc)
{
    GBuffer gbuf;
    gbuf.diffColor = enc.gbuf0;
    gbuf.normal    = enc.gbuf1.xyz;
    gbuf.depth     = enc.gbuf1.w;
    return gbuf;
}

public GBuffer SampleGBuffer(GBufferTexHandles handles, float2 uv)
{
    GBufferEncoded enc;
    enc.gbuf0 = handles.gbuf0.Sample(g_ClampAnisoSampler, uv);
    enc.gbuf1 = handles.gbuf1.Sample(g_ClampAnisoSampler, uv);
    return DecodeGBuffer(enc);
}

// For compute shaders
public GBuffer SampleGBuffer(GBufferTexHandles handles, uint2 screen_coord)
{
    GBufferEncoded enc;
    enc.gbuf0 = handles.gbuf0.Load(int3(screen_coord, 0));
    enc.gbuf1 = handles.gbuf1.Load(int3(screen_coord, 0));
    return DecodeGBuffer(enc);
}

// --------------------------------------------------------------
uint PackUNorm(float input, uint shift, uint num_bits, float dither)
{
    uint mask = num_bits == 32 ? 0xFFFFFFFF : ((1U << num_bits) - 1);
    return uint(saturate(input) * float(mask) + dither) << shift;
}

float UnpackUNorm(uint enc_input, uint shift, uint num_bits)
{
    uint mask = num_bits == 32 ? 0xFFFFFFFF : ((1U << num_bits) - 1);
    enc_input = (enc_input >> shift) & mask;
    return float(enc_input) / float(mask);
}