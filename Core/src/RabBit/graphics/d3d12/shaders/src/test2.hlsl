struct vertexInfo
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct v2p
{
    float4 position : SV_POSITION;
    float3 color    : TEXCOORD0;
};