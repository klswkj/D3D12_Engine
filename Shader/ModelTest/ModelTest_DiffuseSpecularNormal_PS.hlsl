#define EXIST_DIFFUSE_TEXTURE
#define EXIST_SPECULAR_TEXTURE
#define EXIST_NORMAL_TEXTURE

cbuffer ControllerConstants : register(b1) // All of them
{
    uint UseGlossAlpha;
    uint UseSpecularMap;
    float SpecularWeight;
    float SpecularGloss;
    float3 SpecularColor;  // starts a new vector
    uint UseNormalMap;    
    float NormalMapWeight; // starts a new vector
    // float3 CustomPadding;
}

#include "ModelTest_PS.hlsli"