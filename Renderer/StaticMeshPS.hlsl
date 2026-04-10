#include "Common.hlsli"

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    
    uint instanceID : TEXCOORD1;
};

float3 ToneMapReinhard(float3 hdr)
{
    return hdr / (hdr + 1.0); // 컴포넌트별
}

float4 main(VSOutput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 light = normalize(-lightDir);
    
    // diffuse
    float3 lightColor = float3(1.0f, 1.0f, 1.0f); // white light
    float3 diffuse = max(dot(normal, light), 0.0f) * lightColor;

    // ambient
    float3 ambientColor = float3(0.4f, 0.6f, 1.0f); // light blue
    float ambientStrength = 0.15f;
    float3 ambient = ambientColor * ambientStrength;
 
    // sample albedo 
    float4 albedo = g_Texture2D[0].Sample(sampler0, input.uv);
    
    // color
    float3 color = albedo.rgb * (diffuse + ambient);
    
    // gamma correction
    color = pow(color, 1.0f / 2.2f);
    
    return float4(color, albedo.a);
}