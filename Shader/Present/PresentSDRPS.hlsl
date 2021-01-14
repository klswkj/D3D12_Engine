#include "../Miscellaneous/ShaderUtility.hlsli"
#include "PresentRS.hlsli"

Texture2D<float3> ColorTex : register(t0);

[RootSignature(Present_RootSignature)]
float4 main( float4 position : SV_Position ) : SV_Target0
{
    float3 LinearRGB = RemoveDisplayProfile(ColorTex[(int2)position.xy], LDR_COLOR_FORMAT);
    return float4(ApplyDisplayProfile(LinearRGB, DISPLAY_PLANE_FORMAT).xyz, 0);
}
