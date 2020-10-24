#define EXIST_DIFFUSE_TEXTURE 1
#define EXIST_NORMAL_TEXTURE 1

cbuffer ControllerConstants : register(b1) // Diffuse, Normal
{
    float SpecularWeight;
    float SpecularGloss;
    uint useNormalMap;
    float normalMapWeight;
    float3 SpecularColor; // Starts a new vector
}

#include "ModelTest_PS.hlsli"