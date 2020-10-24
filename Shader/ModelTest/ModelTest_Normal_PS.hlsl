#define EXIST_NORMAL_TEXTURE 1

cbuffer ControllerConstants : register(b1) // Only Normal
{
    float3 DiffuseColor;
    float SpecularWeight; // Starts a new vector
    float3 SpecularColor; 
    float SpecularGloss;
    uint useNormalMap; // Starts a new vector
    float normalMapWeight; // Starts a new vector
}

#include "ModelTest_PS.hlsli"