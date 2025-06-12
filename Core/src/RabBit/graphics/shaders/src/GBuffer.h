#ifndef RB_SHADER_GBUFFER
#define RB_SHADER_GBUFFER

struct GBuffer
{
	float4 color;
	float3 normal;
	float  depth;
};

struct GBufferTexIndices
{
	uint gbuf0;
	uint gbuf1;
};

struct GBufferEncoded
{
	float4 gbuf0;
	float4 gbuf1;
};

GBufferEncoded EncodeGBuffer(GBuffer gbuf)
{
	GBufferEncoded enc;
	enc.gbuf0 = gbuf.color;
	enc.gbuf1 = float4(gbuf.normal.rgb, gbuf.depth);
	return enc;
}

GBuffer DecodeGBuffer(GBufferEncoded enc)
{
	GBuffer gbuf;
	gbuf.color  = enc.gbuf0;
	gbuf.normal = enc.gbuf1.xyz;
	gbuf.depth  = enc.gbuf1.w;
	return gbuf;
}

GBuffer SampleGBuffer(GBufferTexIndices textures, float2 uv)
{
	GBufferEncoded enc;
	enc.gbuf0 = FetchTex2D(textures.gbuf0).Sample(g_ClampAnisoSampler, uv);
	enc.gbuf1 = FetchTex2D(textures.gbuf1).Sample(g_ClampAnisoSampler, uv);
	return DecodeGBuffer(enc);
}

// For compute shaders
GBuffer SampleGBuffer(GBufferTexIndices textures, uint2 screen_coord)
{
	GBufferEncoded enc;
	enc.gbuf0 = FetchTex2D(textures.gbuf0).SampleLevel(g_ClampAnisoSampler, screen_coord, 0);
	enc.gbuf1 = FetchTex2D(textures.gbuf1).SampleLevel(g_ClampAnisoSampler, screen_coord, 0);
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

#endif