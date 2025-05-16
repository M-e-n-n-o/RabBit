#ifndef RB_SHADER_GBUFFER
#define RB_SHADER_GBUFFER

struct GBuffer
{
	float4 color;
	float4 normal;
};

struct GBufferEncoded
{
	float4 gbuf0;
	float4 gbuf1;
};

struct GBufferTextures
{
	Texture2D gbuf0;
	Texture2D gbuf1;
};

GBufferEncoded SampleGBuffer(GBufferTextures textures, float2 uv)
{
	GBufferEncoded enc;
	enc.gbuf0 = textures.gbuf0.Sample(g_ClampAnisoSampler, uv);
	enc.gbuf1 = textures.gbuf1.Sample(g_ClampAnisoSampler, uv);
	return enc;
}

GBufferEncoded EncodeGBuffer(GBuffer gbuf)
{
	GBufferEncoded enc;
	enc.gbuf0 = gbuf.color;
	enc.gbuf1 = gbuf.normal;
	return enc;
}

GBuffer DecodeGBuffer(GBufferEncoded enc)
{
	GBuffer gbuf;
	gbuf.color = enc.gbuf0;
	gbuf.normal = enc.gbuf1;
	return gbuf;
}

#endif