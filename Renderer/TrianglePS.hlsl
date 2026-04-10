cbuffer Camera : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float3 lightDir;
    float padding;
};

Texture2D albedoTex[64] : register(t0);
SamplerState sampler0 : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};


float4 main(VSOutput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 light = normalize(-lightDir);
    float diffuse = max(dot(normal, light), 0.0f);
    float ambient = 0.2f;
    float4 albedo = albedoTex[0].Sample(sampler0, input.uv);
    //float4 albedo = float4(1.0f, 0.5f, 0.0f, 1.0f); // Orange color for testing
    return float4(albedo.rgb * (diffuse + ambient), albedo.a);
}