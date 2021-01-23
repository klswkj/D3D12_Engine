Texture2D tex;
SamplerState splr;

cbuffer Kernel
{
    uint nTaps;
    float coefficients[15];
}

cbuffer Control
{
    bool horizontal;
}

float4 main(float2 uv : Texcoord) : SV_Target0
{
    float width, height;
    tex.GetDimensions(width, height);
    float dx, dy;
    if (horizontal)
    {
        dx = 1.0f / width;
        dy = 0.0f;
    }
    else
    {
        dx = 0.0f;
        dy = 1.0f / height;
    }
    const int r = nTaps / 2;
    
    float accAlpha = 0.0f;
    float3 maxColor = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = -r; i <= r; i++)
    {
        const float2 texcoord = uv + float2(dx * i, dy * i);
        const float4 sampledColor = tex.Sample(splr, texcoord).rgba;
        const float coef = coefficients[i + r];
        accAlpha += sampledColor.a * coef;
        maxColor = max(sampledColor.rgb, maxColor);
    }
    maxColor = max(sampledColor.rgb, maxColor);
    return float4(maxColor, accAlpha);
    // return float4(maxColor, 0.0f);
}