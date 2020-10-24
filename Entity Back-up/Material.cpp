#include "stdafx.h"
#include "Material.h"
#include <assimp/material.h>
#include <assimp/types.h>

#include "PSO.h"
#include "RootSignature.h"
#include "Device.h"
#include "PreMadePSO.h"
#include "BufferManager.h"
#include "SamplerDescriptor.h"
#include "TextureManager.h"

#include "Technique.h"
#include "ObjectFilterFlag.h"
#include "eLayoutFlags.cpp"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GeneralPurposePS.h"      // Pseudo name. Invalid
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GeneralPurposeVS.h"      // Pseudo name
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/GeneralPurposePS.h"    // Pseudo name
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/GeneralPurposeVS.h"    // Pseudo name
#endif

std::mutex Material::sm_mutex;

Material::Material(aiMaterial& material, const std::filesystem::path& path) DEBUG_EXCEPT
	: m_ModelFilePath(path.string())
{
	const std::basic_string<char> rootPath = path.parent_path().string() + "\\";
	{
		aiString tempName;
		ASSERT(material.Get(AI_MATKEY_NAME, tempName) == aiReturn_SUCCESS);
		m_Name = tempName.C_Str();
	}
	
	aiString DiffuseFileName;
	aiString SpecularFileName;
	aiString NormalFileName;

	BOOL bHasTexture = false;
	BOOL bHasGlossAlpha = false;
	
	{
		Technique PrePass("PrePass");

		Step _ShadowPrePass("ShadowPrePass");
		PrePass.PushBackStep(std::move(_ShadowPrePass));

		Step _Z_PrePass("Z_PrePass");
		PrePass.PushBackStep(std::move(_Z_PrePass));

		m_Techniques.push_back(std::move(PrePass));
	}
	{
		Technique _ShadowPass("ShadowPass");

		Step _ShadowMappingPass("ShadowMappingPass");
		_ShadowPass.PushBackStep(std::move(_ShadowMappingPass));

		m_Techniques.push_back(std::move(_ShadowPass));
	}
	
	{
		static auto pfnPushBackTexture = [](std::basic_string<wchar_t> path, ePixelLayoutFlags flag, Step& _Step, UINT RootIndex, UINT Offset)
		{
			const ManagedTexture* tempTexture = TextureManager::LoadWICFromFile(path, RootIndex, Offset);

			std::lock_guard<std::mutex> LockGuard(sm_mutex);
			_Step.PushBack(std::make_shared<ManagedTexture>(*tempTexture));
		};

		Technique Phong("Phong");
		Step Lambertian("MainRenderPass");

		// Select out Material
		{
			m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_POSITION_F3; // VertexLayout
			m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_NORMAL_F3;   // VertexLayout

			// Diffuse
			{
				if (material.GetTexture(aiTextureType_DIFFUSE, 0, &DiffuseFileName) == aiReturn_SUCCESS)
				{
					bHasTexture = true;

					m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_TEXCOORD_F2; // VertexLayout

					pfnPushBackTexture(StringToWString(rootPath) + AnsiToWString(DiffuseFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_DIFFUSE_TEXTURE_F2, Lambertian, 2u, 0u);
				}
				else
				{
					m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_MATERIAL_COLOR_F3; // PixelLayout
				}
				// PhongPSO = premade::g_RasterizerTwoSided;
			}

			// Specular
			{
				if (material.GetTexture(aiTextureType_SPECULAR, 0, &SpecularFileName) == aiReturn_SUCCESS)
				{
					bHasTexture = true;
					m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_TEXCOORD_F2;       // VertexLayout

					m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_GLOSS_ALPHA_B1; // PixelLayout
					m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_MAP_B1;         // PixelLayout

					pfnPushBackTexture(StringToWString(rootPath) + AnsiToWString(SpecularFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_SPEC_TEXTURE_F2, Lambertian, 2u, 1u);
				}
				m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_COLOR_F3;   // PixelLayout
				m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_WEIGHT_F1;  // PixelLayout
				m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_GLOSS_F1;   // PixelLayout
			}

			// normal
			{
				if (material.GetTexture(aiTextureType_NORMALS, 0, &NormalFileName) == aiReturn_SUCCESS)
				{
					m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_TEXCOORD_F2;           // VertexLayout
					m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_NORMAL_TANGENT_F3;     // VertexLayout
					m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_NORMAL_BITANGENT_F3;   // VertexLayout

					m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_NORMAL_B1;               // PixelLayout  // RootConstant
					m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_NORMAL_WEIGHT_F1;        // PixelLayout  // RootConstant

					pfnPushBackTexture(StringToWString(rootPath) + AnsiToWString(NormalFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_NORMAL_TEXTURE_F2, Lambertian, 2u, 3u);
				}
			}
		} // End Select out Material

		// TODO : NormalWeight, 그런 세세한거 없음. -> HLSL에 여기서 에러날꺼임
		// 목록
		// 1. MaterialColor
		// 2. UseGlossAlpha
		// 3. UseGlossAlpha
		// 4. UseSpecularMap
		// 5. UseSpecularWeight
		// 6. UseNormalMap
		// 7. UseNormalWeight

		// Set CustomBuffer
		{
			if (bHasTexture)
			{
				// Nothing do.
			}
			else
			{
				m_pMaterialColor = &m_CustomBuffer.m_Data.MaterialColor;
				*m_pMaterialColor = { 0.45f, 0.45f, 0.45f };
			}

			{
				if ((uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_GLOSS_ALPHA_B1 && m_PixelInputLayoutFlag)
				{
					m_pUseGlossAlpha = &m_CustomBuffer.m_Data.UseGlossAlpha;
					*m_pUseGlossAlpha = true;

					m_pUseSpecularMap = &m_CustomBuffer.m_Data.UseSpecularMap;
					*m_pUseGlossAlpha = true;
				}

				material.Get(AI_MATKEY_COLOR_SPECULAR, m_SpecularColor);

				m_SpecularGloss = 8.0f;
				material.Get(AI_MATKEY_SHININESS, m_SpecularGloss);

				m_SpecularWeight = 1.0f;
			}

			{
				if ((uint32_t)ePixelLayoutFlags::ATTRIB_MASK_NORMAL_B1 && m_PixelInputLayoutFlag)
				{
					BOOL* m_UseNormalMap = &m_CustomBuffer.m_Data.UseNormalMap;
					*m_UseNormalMap = true;

					float* m_pNormalMapWeight = &m_CustomBuffer.m_Data.NormalMapWeight;
					*m_pNormalMapWeight = 1.0f;
				}
			}

			// Lambertian.PushBack(std::make_shared<CustomBuffer>(m_CustomBuffer));

			m_InputElements.reserve(5);

			// Set InputElements
			// TODO : Must be Test
			{
				size_t i = 1;
				size_t elementIndex = 0;

				while ((i < (size_t)eVertexLayoutFlags::ATTRIB_MASK_COUNT) && (i & m_VertexInputLayoutFlag))
				{
					m_InputElements.push_back(premade::g_InputElements[elementIndex++]);

					i <<= 1;
				}
			}

			Phong.PushBackStep(std::move(Lambertian));
		} // End Set CustomBuffer

		m_Techniques.push_back(std::move(Phong));
	}

	// BlurOutLine
	{
	    // 
    }
}