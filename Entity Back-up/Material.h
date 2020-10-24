#pragma once
#include "stdafx.h"
#include "Technique.h"
#include "UAVBuffer.h"
#include "ManagedTexture.h"
#include "PSO.h"
#include "VariableConstantBuffer.h"

struct aiMaterial;
struct aiMesh;

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

    CustomBuffer GetCustomBuffer() const noexcept;
    std::vector<Technique> GetTechniques() const noexcept;

private:
    // GraphicsPSO m_PSO;
	std::vector<Technique> m_Techniques;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElements;
	std::string m_ModelFilePath;  // must be std::string
	std::string m_Name;           // must be std::string

    DirectX::XMFLOAT3 m_SpecularColor   { };
    float m_SpecularGloss               = 0;
    float m_SpecularWeight              = 0;
    BOOL* m_pUseGlossAlpha              = nullptr;
    BOOL* m_pUseSpecularMap             = nullptr;
    BOOL* m_UseNormalMap                = nullptr;
    float* m_pNormalMapWeight           = nullptr;
    DirectX::XMFLOAT3* m_pMaterialColor = nullptr;

    uint32_t m_VertexInputLayoutFlag = 0;
    uint32_t m_PixelInputLayoutFlag = 0;

    CustomBuffer m_CustomBuffer;

    static std::mutex sm_mutex;
};

inline CustomBuffer Material::GetCustomBuffer() const noexcept
{
    return m_CustomBuffer;
}

inline std::vector<Technique> Material::GetTechniques() const noexcept
{
    return m_Techniques;
}