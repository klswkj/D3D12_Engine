#define EXIST_NONE_TEXTURE 1

// Has None Texture
cbuffer ControllerConstants : register(b1)
{
    float3 DiffuseColor;
    float SpecularWeight;
    float3 SpecularColor;
    float SpecularGloss;
}

#include "ModelTest_PS.hlsli"