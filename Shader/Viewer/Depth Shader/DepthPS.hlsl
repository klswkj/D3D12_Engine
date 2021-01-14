#include "../Utility/ViewerRS.hlsli"

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TexCoord0;
};

Texture2D<float4> texDiffuse : register(t0);
SamplerState      sampler0   : register(s0);

[RootSignature(Viewer_RootSignature)]
void main(VSOutput vsOutput)
{
    if (texDiffuse.Sample(sampler0, vsOutput.uv).a < 0.5)
    {
        // discard;
    }
}
