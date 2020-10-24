#define EXIST_SPECULAR_TEXTURE 1

cbuffer ControllerConstants : register(b1) // Only Specular
{
    float3 DiffuseColor;
    uint UseGlossAlpha; // Starts a new vector
    uint UseSpecularMap;
    float SpecularWeight;
    float SpecularGloss;
    float3 SpecularColor; // Starts a new vector
}

#include "ModelTest_PS.hlsli"