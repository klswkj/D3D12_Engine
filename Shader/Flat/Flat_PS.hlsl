#include "Flat_RS.hlsli"

cbuffer PSConstants : register(b1)
{
    float3 materialColor;
};

[RootSignature(FlatRootSignature)]
float4 main() : SV_TARGET
{
    return float4(materialColor, 1.0f);
}
