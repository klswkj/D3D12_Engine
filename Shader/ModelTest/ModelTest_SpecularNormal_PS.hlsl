#define EXIST_SPECULAR_TEXTURE 1
#define EXIST_NORMAL_TEXTURE 1

cbuffer ControllerConstants : register(b1) // Specular, Normal
{
    float3 DiffuseColor;
    uint UseGlossAlpha;
    uint UseSpecularMap;   // Starts a new vector
    float SpecularWeight;
    float SpecularGloss;
    float NormalMapWeight; 
    float3 SpecularColor;  // Starts a new vector
    uint UseNormalMap;
}

#include "ModelTest_PS.hlsli"