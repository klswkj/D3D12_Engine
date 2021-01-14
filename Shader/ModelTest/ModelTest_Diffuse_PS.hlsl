#define EXIST_DIFFUSE_TEXTURE 1

cbuffer ControllerConstants1 : register(b1) // Only Diffuse
{
    float3 SpecularColor;
    float SpecularWeight;
    float SpecularGloss;
    // float3 CustomPadding;
}

#include "ModelTest_PS.hlsli"