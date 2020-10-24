#define EXIST_DIFFUSE_TEXTURE 1
#define EXIST_SPECULAR_TEXTURE 1
#define EXIST_NORMAL_TEXTURE 1

cbuffer ControllerConstants : register(b1) // All of them
{
    uint UseGlossAlpha;
    uint UseSpecularMap;
    float SpecularWeight;
    float SpecularGloss;
    float3 SpecularColor;  // starts a new vector
    uint useNormalMap;    
    float normalMapWeight; // starts a new vector
}

#include "ModelTest_PS.hlsli"