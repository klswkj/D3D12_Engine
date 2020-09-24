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
/*
// Material(Technique(Step(RenderingResource))) => Technique => Drawable

// 1. LightPrePass -> Material에서 준비 필요없음.
// 2. ShadowPrePass(RenderLightShadows) -> ****Model 컨테이너 필요****, LightPrePass들의 버퍼들 필요.
//    -> 여기서 갈림길. Choice1) LightPrePass들을 Global하게 끄집어낸다.
//                     Choice2) MasterRederGraph에서 검색해서 레퍼런스 LightPrePass를 받아서 버퍼들을 사용한다.
// 3. Z-PrePass -> 현재의 g_SceneDepthBuffer에 깊이값 기록. ****Model 컨테이너 필요****
// 4. SSAOPASS -> LinearDepth에 값 기록.
// 5. FillLightGridPass -> m_LightBuffer랑 m_LinearDepth읽어서, m_LightGrid랑 m_LightGridBitMask에 값저장
// 6. ShadowMappingPass -> ****Model들 필요함****
// ** 여기서 SSAO 끝났는지 확인. ** 
// 7. MainRenderPass -> ****Model들 필요함****
// 8. OutlineMaskGenerationPass, OutlineDrawing, OutLineBlurPass까지  -> ****Model들 필요함****
// 9. 보너스로 WireFramePass까지 딱.  -> ****Model들 필요함****

// RenderingResource의 ID3D12RootSignature, ID3D12PipelineStateObject의 바이너리 오브젝트들이
// custom::RootSignature, ComputePSO, GraphicsPSO 안에서 어떻게 관리되는지, 
// RootSignature에서 ID3D12RootSIgnature는 바이너리로 가지고 있고,
// PSO에서는 custom::RootSignature의 포인터만을 가지고 있음.
// CommandContext까지 가서야, CommandContext::SetPSO()해야지 ID3D12PipelineStateObject를 가져오고,
// (GraphicsContext나, ComputeContext)::SetRootSignature해야지 그 때서야 포인터 가져옴.

// 결론
// Pass 내에서 RootSignature나, PSO를 멤버변수로 만들어서 사용해야지 된다.

// 다시 결론
// MasterRenderGraph에서 만들거나, Pass내에서 만들어서 쓰는걸로,
// 같이 쓰는거는 MasterRenderGraph에서 쓰고, ComputePass처럼 특수한 경우는 Pass내에서 만들어서

// 현재는 Material에서 파싱하면, Pass내에 어떻게 담고, 
// Material어떤게 있는지 어떻게 전할지 궁리
*/
Material::Material(aiMaterial& material, const std::filesystem::path& path) DEBUG_EXCEPT
	: m_ModelFilePath(path.string())
{
	// const std::basic_string rootPath = path.parent_path().string() + "\\";
	// path.parent_path().generic_wstring()
	// const std::basic_string<wchar_t> rootPath = path.parent_path().string() + "\\";
	const std::basic_string<char> rootPath = path.parent_path().string() + "\\";
	{
		// path.parent_path().generic_wstring() 
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