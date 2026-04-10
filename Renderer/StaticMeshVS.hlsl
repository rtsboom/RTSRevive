#include "Common.hlsli"

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;    
    uint instanceID : TEXCOORD1;
};

struct VertexNormalUV
{
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

VSOutput main(float3 pos : POSITION, VertexNormalUV normalUV, uint instanceID : SV_InstanceID)
{    
    float3 normal = normalUV.normal;
    float2 uv = normalUV.uv;
    float4x4 world = g_InstanceData[instanceID].World;
    float4 worldPos = mul(float4(pos, 1.0f), world);
    
    VSOutput output;
    output.pos = mul(mul(worldPos, view), proj);
    output.normal = mul(float4(normal, 0.f), world).xyz;
    output.uv = uv;
    output.instanceID = instanceID;
    
    return output;
}