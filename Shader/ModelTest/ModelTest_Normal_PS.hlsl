#define EXIST_NORMAL_TEXTURE 1

cbuffer ControllerConstants : register(b1) // Only Normal
{
    float3 DiffuseColor;
    float SpecularWeight; // Starts a new vector
    float3 SpecularColor; 
    float SpecularGloss;
    uint UseNormalMap; // Starts a new vector
    float NormalMapWeight; // Starts a new vector
    // float2 CustomPadding;
}

#include "ModelTest_PS.hlsli"