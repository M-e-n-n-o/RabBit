#ifndef RB_SHADER_GBUFFER
#define RB_SHADER_GBUFFER

struct GBuffer
{
	float4 color;
	float3 normal;
};

struct GBufferEncoded
{

};

GBufferEncoded EncodeGBuffer(GBuffer gbuf)
{

}

GBuffer DecodeGBuffer(GBufferEncoded encoded)
{

}

#endif