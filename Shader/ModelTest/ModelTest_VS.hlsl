#include "ModelTest_RS.hlsli"

cbuffer VSConstants : register(b0)
{
    float4x4 modelToProjection; // IBaseCamera.GetViewProjMatrix(); // VP
    float4x4 ViewMatrix;        // V
    float4x4 modelToShadow;
    float3   ViewerPos;         // 
};

// For TransformConstants.cpp
cbuffer TransformConstants : register(b1) 
{
    float4x4 model;
}

struct VSInput
{
    float3 position  : POSITION;
    float3 normal    : NORMAL;
    float2 texcoord0 : TEXCOORD;
    float3 tangent   : TANGENT;
    float3 bitangent : BITANGENT;
};

struct VSOutput
{
    float4 position    : SV_Position;
    float3 worldPos    : WorldPos;
    float3 normal      : Normal;
    float2 texCoord    : TexCoord0;
    float3 viewDir     : TexCoord1;
    float3 shadowCoord : TexCoord2;
    float3 tangent     : Tangent;
    float3 bitangent   : Bitangent;
};

[RootSignature(ModelTest_RootSignature)]
VSOutput main(VSInput vsInput)
{
    VSOutput vsOutput;
    
    vsOutput.position    = mul(modelToProjection, mul(model, float4(vsInput.position, 1.0f)));
    vsOutput.worldPos    = vsInput.position;
    vsOutput.normal      = mul(ViewMatrix, mul(model, float4(vsInput.normal, 1.0f))).xyz;
    vsOutput.texCoord    = vsInput.texcoord0;
    vsOutput.viewDir     = mul(modelToProjection, mul(model, float4(vsInput.position, 1.0f))).xyz - ViewerPos;
    vsOutput.shadowCoord = vsOutput.position.xyz;
    vsOutput.tangent     = mul(ViewMatrix, mul(model, float4(vsInput.tangent,   1.0f))).xyz;
    vsOutput.bitangent   = mul(ViewMatrix, mul(model, float4(vsInput.bitangent, 1.0f))).xyz;
    
    // Original Code.
    /*
    vsOutput.position    = mul(modelToProjection, float4(vsInput.position, 1.0));
    vsOutput.worldPos    = vsInput.position;
    vsOutput.normal      = vsInput.normal;
    vsOutput.texCoord    = vsInput.texcoord0;
    vsOutput.viewDir     = vsInput.position - ViewerPos;
    vsOutput.shadowCoord = mul(modelToShadow, float4(vsInput.position, 1.0)).xyz;
    vsOutput.tangent     = vsInput.tangent;
    vsOutput.bitangent   = vsInput.bitangent;
    */
    
    return vsOutput;
}