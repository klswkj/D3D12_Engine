#include "PresentRS.hlsli"

cbuffer Constants : register(b0)
{
    float2 RcpDestDim;
}
Texture2D ColorTex : register(t0);

SamplerState BilinearSampler : register(s0);

[RootSignature(Present_RootSignature)]
float4 main( float4 position : SV_Position ) : SV_Target0
{
    //float2 UV = saturate(RcpDestDim * position.xy);
    //return ColorTex.SampleLevel(BilinearSampler, UV, 0);
    return ColorTex[(int2)position.xy];
}
