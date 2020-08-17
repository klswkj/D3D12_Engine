cbuffer CSolidColor : register(b0)
{
    float3 solidColor;
}

float4 main() : SV_TARGET
{
    return float4(solidColor, 1.0f);
}