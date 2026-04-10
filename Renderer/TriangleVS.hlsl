cbuffer Camera : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float3 lightDir;
    float padding;
};

struct InstanceData
{
    float4x4 world;
};

struct VertexNormalUV
{
    float nx, ny, nz; 
    float u, v;
};

StructuredBuffer<InstanceData> instanceBuffer : register(t1);
StructuredBuffer<float3> positions : register(t2);
StructuredBuffer<VertexNormalUV> normalUVs : register(t3);


struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


VSOutput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    float3 pos = positions[vertexID];
    VertexNormalUV normalUV = normalUVs[vertexID];
    float3 normal = float3(normalUV.nx, normalUV.ny, normalUV.nz);
    float2 uv = float2(normalUV.u, normalUV.v);
    
    VSOutput output;
    
    float4x4 world = instanceBuffer[instanceID].world;
    float4 worldPos = mul(float4(pos, 1.0f), world);
    output.pos = mul(mul(worldPos, view), proj);
    output.normal = mul(float4(normal, 0.0f), world).xyz;
    output.uv = uv;

    return output;
}
