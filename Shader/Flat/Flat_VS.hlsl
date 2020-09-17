cbuffer TransformCBuf : register(b0)
{
    float4x4 model;         // Position
    float4x4 modelView;     // ViewProjectionMatrix => model * [ DirectX::XMMatrixLookToLH(Position, Forward, Up) == ViewMatrix ]
    float4x4 modelViewProj; // ModelView * XMMATRIXPerspectiveLH(width, height, nearZ, farZ)
};

float4 main( float3 pos : POSITION ) : SV_POSITION
{
    return mul(float4(pos, 1.0f), modelViewProj);
}