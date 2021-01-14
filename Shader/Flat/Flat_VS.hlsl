cbuffer TransformCBuf : register(b0)
{
    float4x4 modelViewProj; // ModelView * XMMATRIXPerspectiveLH(width, height, nearZ, farZ)
};

float4 main(float3 pos : POSITION) : SV_POSITION
{
    return mul(modelViewProj, float4(pos, 1.0f));
}