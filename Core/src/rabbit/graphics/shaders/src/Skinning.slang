module Skinning;

ConstantBuffer<uint> g_VertexCount;

struct SkinVertex
{
    uint2  bones;
    float4 weights;
}

struct SecondaryVertex
{
    float3 normal;
    float2 uv;
};

struct Handles
{
    StructuredBuffer<float3>.Handle            inPositions;
    StructuredBuffer<SecondaryVertex>.Handle   inSecondary;
    StructuredBuffer<SkinVertex>.Handle        inSkinVerts;
    StructuredBuffer<float4x4>.Handle          boneMatrices;

    RWStructuredBuffer<float3>.Handle          outPositions;
    RWStructuredBuffer<SecondaryVertex>.Handle outSecondary;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void ApplySkinning(uint2 dispatch_id : SV_DispatchThreadID,
                   uniform Handles handles)
{
    uint vi = dispatch_id.x;
    if (vi >= g_VertexCount)
    {
        return;
    }

    float3 local_pos = handles.inPositions[vi];
    SecondaryVertex sv_secondary = handles.inSecondary[vi];
    SkinVertex sv = handles.inSkinVerts[vi];

    float3 skinned_pos = float3(0, 0, 0);
    float3 skinned_normal = float3(0, 0, 0);

    [unroll]
    for (uint i = 0; i < 4; i++)
    {
        uint bone_idx = (sv.bones[i / 2] >> ((i % 2) * 16)) & 0xFFFF;
        float w = sv.weights[i];
        if (w > 0.0f)
        {
            skinned_pos    += mul(handles.boneMatrices[bone_idx], float4(local_pos, 1.0f)).xyz * w;
            skinned_normal += mul(handles.boneMatrices[bone_idx], float4(sv_secondary.normal, 0.0f)).xyz * w;
        }
    }

    sv_secondary.normal = normalize(skinned_normal);

    handles.outPositions[vi] = skinned_pos;
    handles.outSecondary[vi] = sv_secondary;
}