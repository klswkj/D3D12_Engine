/*
#define VERTEX_INPUT_POSITION_F3 1
#define VERTEX_INPUT_NORMAL_F3   1
#define VERTEX_INPUT_TEXCOORD_F2 1
#define VERTEX_INPUT_NORMAL      1
*/
#include "../Utility/ViewerRS.hlsli"

cbuffer VSConstants : register(b0)
{
    float4x4 modelToProjection;
    float4x4 modelToShadow;
    float3 ViewerPos;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
#ifdef VERTEX_INPUT_TEXCOORD_F2
    float2 texcoord0 : TEXCOORD;
    #ifdef VERTEX_INPUT_NORMAL
        float3 tangent : TANGENT;
        float3 bitangent : BITANGENT;
    #endif
#endif
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 worldPos : WorldPos;
    float3 normal : Normal;
    float3 viewDir : TexCoord1;
    float3 shadowCoord : TexCoord2;
#ifdef VERTEX_INPUT_TEXCOORD_F2
    float2 texCoord : TexCoord0;
    #ifdef VERTEX_INPUT_NORMAL
        float3 tangent : Tangent;
        float3 bitangent : Bitangent;
    #endif
#endif
};

[RootSignature(Viewer_RootSignature)]
VSOutput main(VSInput vsInput)
{
    VSOutput vsOutput;

    vsOutput.position = mul(modelToProjection, float4(vsInput.position, 1.0));
    vsOutput.worldPos = vsInput.position;
    vsOutput.normal = vsInput.normal;
    vsOutput.viewDir = vsInput.position - ViewerPos;
    vsOutput.shadowCoord = mul(modelToShadow, float4(vsInput.position, 1.0)).xyz;
    
#ifdef VERTEX_INPUT_TEXCOORD_F2
    vsOutput.texCoord = vsInput.texcoord0;
    #ifdef VERTEX_INPUT_NORMAL
        vsOutput.tangent = vsInput.tangent;
        vsOutput.bitangent = vsInput.bitangent;
    #endif
#endif
    return vsOutput;
}