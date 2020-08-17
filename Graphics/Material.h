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
    /*
    DirectX::XMFLOAT3 m_SpecularColor{};
    float m_SpecularGloss{ 0 };
    float m_SpecularWeight{ 0 };
    */
    struct MaterialData
    {
        BOOL UseGlossAlpha{ 0 };
        BOOL UseSpecularMap{ 0 };
        BOOL UseNormalMap{ 0 };
        float NormalMapWeight{ 0 };
        DirectX::XMFLOAT3 MaterialColor{ 0 };
    };
public:
	Material(const aiMaterial& material, const std::filesystem::path& path) DEBUG_EXCEPT;
    ~Material() {}
	custom::StructuredBuffer ExtractVertices(const aiMesh& mesh) const noexcept;
	std::vector<unsigned short> ExtractIndices(const aiMesh& mesh) const noexcept;
	std::shared_ptr<custom::StructuredBuffer> MakeVertexBindable(const aiMesh& mesh, float scale = 1.0f) const DEBUG_EXCEPT;
	std::shared_ptr<custom::ByteAddressBuffer> MakeIndexBindable(const aiMesh& mesh) const DEBUG_EXCEPT;
    CustomBuffer GetCustomBuffer() const noexcept
    {
        return m_CustomBuffer;
    }
private:
	// std::string MakeMeshTag(const aiMesh& mesh) const noexcept;
private:

    uint32_t m_VertexInputLayoutFlag{ 0 };
    uint32_t m_PixelInputLayoutFlag{ 0 };
	
    GraphicsPSO m_PSO;
	std::vector<Technique> m_Techniques;
	std::string m_ModelFilePath;  // must be std::string
	std::string m_Name;           // must be std::string

    DirectX::XMFLOAT3 m_SpecularColor{};
    float m_SpecularGloss{ 0 };
    float m_SpecularWeight{ 0 };
    BOOL* m_pUseGlossAlpha{ nullptr };
    BOOL* m_pUseSpecularMap{ nullptr };
    BOOL* m_UseNormalMap{ nullptr };
    float* m_pNormalMapWeight{ nullptr };
    DirectX::XMFLOAT3* m_pMaterialColor{ nullptr };

    CustomBuffer m_CustomBuffer;

    static std::mutex sm_mutex;

	D3D12_CPU_DESCRIPTOR_HANDLE* m_SRVs; // = new D3D12_CPU_DESCRIPTOR_HANDLE[m_Header.materialCount * 6];
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
    DirectX::XMFLOAT3 MaterialColor;    // pixel     -> Texture없으면 씀.
    DirectX::XMFLOAT3 SpecularColor;    // pixel     -> 씀
    float SpecularWeight;               // pixel     -> 씀
    float SpecularGloss;                // pixel     -> 씀
    BOOL bUseGlossAlpha;                // pixel     -> Specular 있으면 씀
    BOOL bUseSpecularMap;               // pixel     -> Specular 있으면 씀
    float NormalMapWeight;              // pixel     -> 노말 있으면 씀
    BOOL bUseNormalMap;                 // pixel     -> 노말 있으면 씀
};

// 이거를 그대로 만들지 않고, 위에서부터 하나하나 차례대로 세서
// 버퍼를 동적할당. 그래서 포인터로 나눠가지기?


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