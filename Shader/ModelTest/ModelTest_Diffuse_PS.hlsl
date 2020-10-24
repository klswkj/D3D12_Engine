#define EXIST_DIFFUSE_TEXTURE 1

cbuffer ControllerConstants1 : register(b1) // Only Diffuse
{
    float3 SpecularColor;
    float SpecularWeight;
    float SpecularGloss;
}

#include "ModelTest_PS.hlsli"