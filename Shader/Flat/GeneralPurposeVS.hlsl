// #include "GeneralPurposeRS.hlsli"

cbuffer VSConstants : register(b0)
{
    float4x4 modelToProjection;
    float4x4 modelToShadow;
    float3 ViewerPosition;
}

float4 main(float3 pos : POSITION) : SV_POSITION
{
    return mul(float4(pos, 1.0f), modelToProjection);
}