#include "OutlineWithBlurRS.hlsli"

cbuffer Kernel : register(b0)
{
    uint diameter;
    float coefficients[15];
}

cbuffer Control : register(b1)
{
    bool horizontal;
}

Texture2D<float> Stencil : register(t0);
SamplerState sampler0 : register(s0);

[RootSignature(OutlineWithBlur_RootSignature)]
float4 main(float2 uv : Texcoord) : SV_Target
{
    float Width, Height;
    Stencil.GetDimensions(Width, Height);
    
    float dx, dy;
    if (horizontal)
    {
        dx = 1.0f / Width;
        dy = 0.0f;
    }
    else
    {
        dx = 0.0f;
        dy = 1.0f / Height;
    }
    const int Radius = diameter / 2;
    
    float AlphaValue = 0.0f;
    float3 maxColor = float3(0.0f, 0.0f, 0.0f);
    for (int i = -Radius; i <= Radius; i++)
    {
        const float2 TextureCoordinate = uv + float2(dx * i, dy * i);
        const float4 SampleStencil = Stencil.Sample(sampler0, TextureCoordinate);
        const float coef = coefficients[i + Radius];
        AlphaValue += SampleStencil.a * coef;
        maxColor = max(SampleStencil.rgb, maxColor);
    }
    return float4(maxColor, AlphaValue);
}