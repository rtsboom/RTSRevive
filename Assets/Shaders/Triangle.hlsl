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

StructuredBuffer<InstanceData> instanceBuffer : register(t1);
StructuredBuffer<float3> positions : register(t2);
StructuredBuffer<float3> normals : register(t3);
StructuredBuffer<float2> uvs : register(t4);

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

PSInput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    float3 pos = positions[vertexID];
    float3 normal = normals[vertexID];
    float2 uv = uvs[vertexID];
    
    PSInput output;
    
    float4x4 world = instanceBuffer[instanceID].world;
    float4 worldPos = mul(float4(pos, 1.0f), world);
    output.pos = mul(mul(worldPos, view), proj);
    output.normal = mul(float4(normal, 0.0f), world).xyz;
    output.uv = uv;
    return output;
}

Texture2D albedoTex : register(t0);
SamplerState sampler0 : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 light = normalize(-lightDir);
    float diffuse = max(dot(normal, light), 0.0f);
    float ambient = 0.2f;
    //float4 albedo = albedoTex.Sample(sampler0, input.uv);
    float4 albedo = float4(1.0f, 0.5f, 0.0f, 1.0f); // Orange color for testing
    return float4(albedo.rgb * (diffuse + ambient), albedo.a);
}