#define EXIST_SPECULAR_TEXTURE 1

cbuffer ControllerConstants : register(b1) // Only Specular
{
    float3 DiffuseColor;
    uint UseGlossAlpha; // Starts a new vector
    uint UseSpecularMap;
    float SpecularWeight;
    float SpecularGloss;
    float CustomPadding0;
    float3 SpecularColor; // Starts a new vector
    // float CustomPadding1;
}

#include "ModelTest_PS.hlsli"