#include "TextRS.hlsli"

cbuffer cbFontParams : register(b0)
{
    float4 Color;
    float2 ShadowOffset;
    float ShadowHardness;
    float ShadowOpacity;
    float HeightRange;    // The range of the signed distance field.
}

Texture2D<float> SignedDistanceFieldTex : register( t0 );
SamplerState LinearSampler : register( s0 );

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float GetAlpha( float2 uv )
{
    return saturate(SignedDistanceFieldTex.Sample(LinearSampler, uv) * HeightRange + 0.5);
}

[RootSignature(Text_RootSignature)]
float4 main( PS_INPUT Input ) : SV_Target
{
    return float4(Color.rgb, 1) * GetAlpha(Input.uv) * Color.a;
}
