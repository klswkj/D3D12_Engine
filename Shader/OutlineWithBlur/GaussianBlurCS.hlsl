cbuffer BlurConstants : register(b0)
{
    float BlurSigma;
    int   BlurRadius;
    int   BlurCount;
    float BlurIntensity;
};

RWTexture2D<float4> BlurResult : register(u0);
RWTexture2D<float4> BlurHelp   : register(u1);

static const int gMaxBlurRadius = 5;
static const float PI = 3.14159265f;

#define N 256
#define CacheSize (N + 2 * gMaxBlurRadius)
groupshared float4 gCache[CacheSize];

float gauss(float x, float Sigma)
{
    const float SS = Sigma * Sigma;
    const float ReturnValue = (float) (1.0 / (float) sqrt(2.0 * PI * SS)) * (float) exp(-(x * x) / (2.0 * SS));
    return ReturnValue;
}

[numthreads(N, 1, 1)]
void main
( 
int3 groupThreadID    : SV_GroupThreadID,
int3 dispatchThreadID : SV_DispatchThreadID
)
{
    float Weights[11];
    
    {
        const uint Diameter = BlurRadius * 2 + 1;
        float CoefficientsSum = 0.0f;
	
        for (uint i = 0; i < Diameter; ++i)
        {
            const float x = float(i - BlurRadius);
            const float Gauss = gauss(x, BlurSigma);
            CoefficientsSum += Gauss;

            Weights[i] = Gauss;
        }

        for (i = 0; i < Diameter; ++i)
        {
            Weights[i] /= CoefficientsSum;
            Weights[i] += BlurIntensity + BlurIntensity * BlurIntensity;
        }
    }
    
    // ThreadX = 3
    // ThreadY = 632
    float ResultBufferWidth  = 0.0f; // 632
    float ResultBufferHeight = 0.0f; // 340
    
    BlurResult.GetDimensions(ResultBufferWidth, ResultBufferHeight);
    
    for (int n = 0; n < BlurCount; ++n)
    {
        // Horizontal Blur
        {
            if (groupThreadID.x < BlurRadius)
            {
                int x = max(dispatchThreadID.x - BlurRadius, 0);
                gCache[groupThreadID.x] = BlurResult[int2(x, dispatchThreadID.y)];
            }
            if (N - BlurRadius <= groupThreadID.x)
            {
                int x = min(dispatchThreadID.x + BlurRadius, BlurResult.Length.x - 1);
                gCache[groupThreadID.x + 2 * BlurRadius] = BlurResult[int2(x, dispatchThreadID.y)];
            }

            gCache[groupThreadID.x + BlurRadius] = BlurResult[min(dispatchThreadID.xy, BlurResult.Length.xy - 1)];

            GroupMemoryBarrierWithGroupSync();
	
            float4 blurColor = float4(0, 0, 0, 0);
	
            for (int i = -BlurRadius; i <= BlurRadius; ++i)
            {
                int k = groupThreadID.x + BlurRadius + i;
		
                blurColor += Weights[i + BlurRadius] * gCache[k];
            }
        
            BlurHelp[dispatchThreadID.xy] = blurColor;
        }
        
        // Vertical Blur
        {
            if (groupThreadID.x < BlurRadius)
            {
                int y = max(dispatchThreadID.x - BlurRadius, 0);
                gCache[groupThreadID.x] = BlurHelp[int2(dispatchThreadID.y, y)];
            }
            if (N - BlurRadius <= groupThreadID.x)
            {
                int y = min(dispatchThreadID.x + BlurRadius, BlurHelp.Length.y - 1);
                gCache[groupThreadID.x + 2 * BlurRadius] = BlurHelp[int2(dispatchThreadID.y, y)];
            }
	
            gCache[groupThreadID.x + BlurRadius] = BlurHelp[min(dispatchThreadID.yx, BlurHelp.Length.xy - 1)];

            GroupMemoryBarrierWithGroupSync();

            float4 blurColor = float4(0, 0, 0, 0);
	
            for (int i = -BlurRadius; i <= BlurRadius; ++i)
            {
                int k = groupThreadID.x + BlurRadius + i;
		
                blurColor += Weights[i + BlurRadius] * gCache[k];
            }
            
            BlurResult[dispatchThreadID.yx] = blurColor;
        }
    }
}