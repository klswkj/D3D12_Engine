// uniform float     iTime = shader playback time (in seconds)
// uniform float3    iResolution = viewport resolution (in pixels)
// uniform samplerXX iChannel = input channel 2D/Cube
//


const float range = 0.05;
const float noiseQuality = 250.0;
const float noiseIntensity = 0.0088;
const float offsetIntensity = 0.02;
const float colorOffsetIntensity = 1.3;

float rand(float2 Input)
{
    const float result = sin(dot(Input.xy, float2(12.9898, 78.233))) * 43758.5453;
    
    return frac(result);
}

float verticalBar(float pos, float uvY, float offset)
{
    float edge0 = (pos - range);
    float edge1 = (pos + range);

    float x = smoothstep(edge0, pos, uvY) * offset;
    x -= smoothstep(pos, edge1, uvY) * offset;
    return x;
}

void main(out float4 fragColor, in float2 fragCoord) : SV_TARGET
{
    float2 uv = fragCoord.xy / iResolution.xy;
    
    for (float i = 0.0; i < 0.71; i += 0.1313)
    {
        float d = (iTime * i) % 1.7;
        float o = sin(1.0 - tan(iTime * 0.24 * i));
        o *= offsetIntensity;
        uv.x += verticalBar(d, uv.y, o);
    }
    
    float uvY = uv.y;
    uvY *= noiseQuality;
    uvY = float(int(uvY)) * (1.0 / noiseQuality);
    float noise = rand(float2(iTime * 0.00001, uvY));
    uv.x += noise * noiseIntensity;

    float2 offsetR = vec2(0.006 * sin(iTime), 0.0) * colorOffsetIntensity;
    float2 offsetG = vec2(0.0073 * (cos(iTime * 0.97)), 0.0) * colorOffsetIntensity;
    
    float r = texture(iChannel0, uv + offsetR).r;
    float g = texture(iChannel0, uv + offsetG).g;
    float b = texture(iChannel0, uv).b;

    float4 tex = float4(r, g, b, 1.0);
    fragColor = tex;
}