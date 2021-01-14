#define EXIST_DIFFUSE_TEXTURE 1
#define EXIST_SPECULAR_TEXTURE 1

cbuffer ControllerConstants : register(b1) // Diffuse, Specular
{
    uint UseGlossAlpha;
    uint UseSpecularMap;
    float SpecularWeight;
    float SpecularGloss;
    float3 SpecularColor; // Starts a new vector
    // float CustomPadding;
}

#include "ModelTest_PS.hlsli"