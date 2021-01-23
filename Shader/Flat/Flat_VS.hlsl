cbuffer TransformCBuf : register(b0)
{
    float4x4 modelToProjection;
};

// For TransformConstants.cpp
cbuffer TransformConstants : register(b1)
{
    float4x4 model;
}

float4 main(float3 position : POSITION) : SV_POSITION
{
    return mul(modelToProjection, mul(model, float4(position, 1.0f)));
}