#pragma once
#include "stdafx.h"
#include "Technique.h"
#include "UAVBuffer.h"
#include "ManagedTexture.h"
#include "PSO.h"
#include "VariableConstantBuffer.h"

class aiMaterial;
class aiMesh;

namespace custom
{
	class ByteAddressBuffer;
}

#ifndef DEBUG_EXCEPT
#define DEBUG_EXCEPT noexcept(!_DEBUG)
#endif

class Material
{
public:
	Material(aiMaterial& material, const std::filesystem::path& path) DEBUG_EXCEPT;
    ~Material() {}
	// custom::StructuredBuffer ExtractVertices(const aiMesh& mesh) const noexcept;
	// std::vector<unsigned short> ExtractIndices(const aiMesh& mesh) const noexcept;
	// std::shared_ptr<custom::StructuredBuffer> MakeVertexBindable(const aiMesh& mesh, float scale = 1.0f) const DEBUG_EXCEPT;
	// std::shared_ptr<custom::ByteAddressBuffer> MakeIndexBindable(const aiMesh& mesh) const DEBUG_EXCEPT;
    CustomBuffer GetCustomBuffer() const noexcept
    {
        return m_CustomBuffer;
    }
private:
	// std::string MakeMeshTag(const aiMesh& mesh) const noexcept;
private:

    uint32_t m_VertexInputLayoutFlag{ 0 };
    uint32_t m_PixelInputLayoutFlag{ 0 };
	
    // GraphicsPSO m_PSO;
	std::vector<Technique> m_Techniques;
	std::string m_ModelFilePath;  // must be std::string
	std::string m_Name;           // must be std::string

    DirectX::XMFLOAT3 m_SpecularColor   { };
    float m_SpecularGloss               { 0 };
    float m_SpecularWeight              { 0 };
    BOOL* m_pUseGlossAlpha              { nullptr };
    BOOL* m_pUseSpecularMap             { nullptr };
    BOOL* m_UseNormalMap                { nullptr };
    float* m_pNormalMapWeight           { nullptr };
    DirectX::XMFLOAT3* m_pMaterialColor { nullptr };

    CustomBuffer m_CustomBuffer;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElements;

    static std::mutex sm_mutex;
};


struct MaterialMaterial // CachingPixelConstantBufferEx
{
    typedef struct
    {
        DirectX::XMMATRIX model;
        DirectX::XMMATRIX modelView;
        DirectX::XMMATRIX modelViewProj;
    }TransformCBuf;
    TransformCBuf Transform;            // Bindable
    DirectX::XMFLOAT3 MaterialColor;    // pixel     -> Texture¾øÀ¸¸é ¾¸.
    DirectX::XMFLOAT3 SpecularColor;    // pixel     -> ¾¸
    float SpecularWeight;               // pixel     -> ¾¸
    float SpecularGloss;                // pixel     -> ¾¸
    BOOL bUseGlossAlpha;                // pixel     -> Specular ÀÖÀ¸¸é ¾¸
    BOOL bUseSpecularMap;               // pixel     -> Specular ÀÖÀ¸¸é ¾¸
    float NormalMapWeight;              // pixel     -> ³ë¸» ÀÖÀ¸¸é ¾¸
    BOOL bUseNormalMap;                 // pixel     -> ³ë¸» ÀÖÀ¸¸é ¾¸
};

/*
struct Material
    {
        Vector3 diffuse;
        Vector3 specular;
        Vector3 ambient;
        Vector3 emissive;
        Vector3 transparent; // light passing through a transparent surface is multiplied by this filter color
        float opacity;
        float shininess; // specular exponent
        float specularStrength; // multiplier on top of specular color

        enum {maxTexPath = 128};
        enum {texCount = 6};
        char texDiffusePath[maxTexPath];
        char texSpecularPath[maxTexPath];
        char texEmissivePath[maxTexPath];
        char texNormalPath[maxTexPath];
        char texLightmapPath[maxTexPath];
        char texReflectionPath[maxTexPath];

        enum {maxMaterialName = 128};
        char name[maxMaterialName];
    };
    Material *m_pMaterial;
*/