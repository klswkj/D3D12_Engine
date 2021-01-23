struct VertexOut
{
    float4 position  : SV_Position;
    float2 texcoord0 : TexCoord0;
};

Texture2D    OutlineTexture      : register(t0);
SamplerState LinearMirrorSampler : register(s0);

float4 main(VertexOut vsOutput) : SV_Target0
{
    const float4 sampledOutlineColor = OutlineTexture.Sample(LinearMirrorSampler, vsOutput.texcoord0).rgba;
    
    return sampledOutlineColor;
}