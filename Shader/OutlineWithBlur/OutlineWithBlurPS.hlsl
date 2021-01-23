#include "OutlineWithBlurRS.hlsli"

#define MaxRadius 5
#define WeightsCount MaxRadius * 2 + 1
static const float PI = 3.14159265f;

struct VertexOut
{
    float4 position : SV_Position;
    float2 texcoord0 : TexCoord0;
};

cbuffer Kernel : register(b0)
{
    // uint diameterAndBlurDirection;
    // float coefficients[15];
    float BlurSigma;
    int   BlurRadius;
    int   BlurDirection; // 0 -> Vertical, 1 -> Horizontal
    float BlurIntensity;
}

Texture2D<float4> OutlineBuffer : register(t0);
SamplerState      LinearMirrorSampler : register(s0);
SamplerState      PointMirrorSampler  : register(s1);

float gauss(float x, float Sigma)
{
    const float SS = Sigma * Sigma;
    const float ReturnValue = (float) (1.0 / (float) sqrt(2.0 * PI * SS)) * (float) exp(-(x * x) / (2.0 * SS));
    return ReturnValue;
}

// [RootSignature(OutlineWithBlur_RootSignature)]
float4 main(VertexOut vsOutput) : SV_Target0
{
    float Weights[11];
    
    {
        const int Diameter = BlurRadius * 2 + 1;
        float CoefficientsSum = 0.0f;
        
        for (int i = 0; i < Diameter; i++)
        {
            float x = float(i - BlurRadius);
            float g = gauss(x, BlurSigma);
            CoefficientsSum += g;
            Weights[i] = g;
        }
        
        for (i = 0; i < Diameter; i++)
        {
            Weights[i] = (float) Weights[i] / CoefficientsSum;
            Weights[i] += BlurIntensity;

        }
    }
    
    float Width, Height;
    OutlineBuffer.GetDimensions(Width, Height);
    
    float dx, dy;
    if (BlurDirection)
    {
        dx = 1.0f / Width;
        dy = 0.0f;
    }
    else
    {
        dx = 0.0f;
        dy = 1.0f / Height;
    }
    
    float AlphaValue = 0.0f;
    float3 MaxColor = float3(0.0f, 0.0f, 0.0f);
    
    // [unroll]
    for (int i = -BlurRadius; i <= BlurRadius; ++i)
    {
        const float2 TextureCoordinate = vsOutput.texcoord0 + float2(dx * i, dy * i);
        
        float4 SampledOutline = float4(0.0f, 0.0f, 0.0f, 0.0f);
        
        if(BlurDirection)
        {
            SampledOutline = OutlineBuffer.Sample(PointMirrorSampler, TextureCoordinate);
        }
        else
        {
            SampledOutline = OutlineBuffer.Sample(LinearMirrorSampler, TextureCoordinate);
        }
        
        AlphaValue += SampledOutline.a * Weights[i + BlurRadius];
        MaxColor    = max(SampledOutline.rgb, MaxColor);
    }
    
    return float4(MaxColor, AlphaValue);
}