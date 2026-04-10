// Common root signature binding definitions for all shaders
#ifndef COMMON_HLSLI
#define COMMON_HLSLI

// Root signature bindings

#define Count_Texture2D 64
#define Count_Texture3D 8

#define	Space_CBufferPerFrame   space0
#define	Space_CBufferPerPass    space1
#define	Space_CBufferPerDraw    space2
#define	Space_Texture2D         space3
#define	Space_Texture3D         space4
#define	Space_InstanceData      space5


struct InstanceData
{
    float4x4 World;
};

Texture2D g_Texture2D[Count_Texture2D] : register(t0, Space_Texture2D);
Texture3D g_texture3D[Count_Texture3D] : register(t0, Space_Texture3D);
StructuredBuffer<InstanceData> g_InstanceData : register(t0, Space_InstanceData);

cbuffer PerFrame : register(b0, Space_CBufferPerFrame)
{    
    float4x4 view;
    float4x4 proj;
    float3 lightDir;
    float pad;
};


SamplerState sampler0 : register(s0);

#endif

