#include "../Miscellaneous/ShaderUtility.hlsli"
#include "PresentRS.hlsli"

Texture2D<float3> ColorTex : register(t0);
SamplerState PointSampler : register(s1);

cbuffer Constants : register(b0)
{
    float ScaleFactor;
}

[RootSignature(Present_RootSignature)]
float4 main( float4 position : SV_Position, float2 uv : TexCoord0 ) : SV_Target0
{
    float2 ScaledUV = ScaleFactor * (uv - 0.5) + 0.5;
    return (float4) (ColorTex.SampleLevel(PointSampler, ScaledUV, 0), 0);
}
