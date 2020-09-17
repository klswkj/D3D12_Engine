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

// Material(Technique(Step(RenderingResource))) => Technique => Drawable

// 1. LightPrePass -> Material���� �غ� �ʿ����.
// 2. ShadowPrePass(RenderLightShadows) -> ****Model �����̳� �ʿ�****, LightPrePass���� ���۵� �ʿ�.
//    -> ���⼭ ������. Choice1) LightPrePass���� Global�ϰ� �������.
//                     Choice2) MasterRederGraph���� �˻��ؼ� ���۷��� LightPrePass�� �޾Ƽ� ���۵��� ����Ѵ�.
// 3. Z-PrePass -> ������ g_SceneDepthBuffer�� ���̰� ���. ****Model �����̳� �ʿ�****
// 4. SSAOPASS -> LinearDepth�� �� ���.
// 5. FillLightGridPass -> m_LightBuffer�� m_LinearDepth�о, m_LightGrid�� m_LightGridBitMask�� ������
// 6. ShadowMappingPass -> ****Model�� �ʿ���****
// ** ���⼭ SSAO �������� Ȯ��. ** 
// 7. MainRenderPass -> ****Model�� �ʿ���****
// 8. OutlineMaskGenerationPass, OutlineDrawing, OutLineBlurPass����  -> ****Model�� �ʿ���****
// 9. ���ʽ��� WireFramePass���� ��.  -> ****Model�� �ʿ���****

// RenderingResource�� ID3D12RootSignature, ID3D12PipelineStateObject�� ���̳ʸ� ������Ʈ����
// custom::RootSignature, ComputePSO, GraphicsPSO �ȿ��� ��� �����Ǵ���, 
// RootSignature���� ID3D12RootSIgnature�� ���̳ʸ��� ������ �ְ�,
// PSO������ custom::RootSignature�� �����͸��� ������ ����.
// CommandContext���� ������, CommandContext::SetPSO()�ؾ��� ID3D12PipelineStateObject�� ��������,
// (GraphicsContext��, ComputeContext)::SetRootSignature�ؾ��� �� ������ ������ ������.

// ���
// Pass ������ RootSignature��, PSO�� ��������� ���� ����ؾ��� �ȴ�.

// �ٽ� ���
// MasterRenderGraph���� ����ų�, Pass������ ���� ���°ɷ�,
// ���� ���°Ŵ� MasterRenderGraph���� ����, ComputePassó�� Ư���� ���� Pass������ ����

// ����� Material���� �Ľ��ϸ�, Pass���� ��� ���, 
// Material��� �ִ��� ��� ������ �ø�

Material::Material(aiMaterial& material, const std::filesystem::path& path) DEBUG_EXCEPT
	: m_ModelFilePath(path.string())
{
	// const std::basic_string rootPath = path.parent_path().string() + "\\";
	// path.parent_path().generic_wstring()
	const std::basic_string<wchar_t> rootPath = path.parent_path().generic_wstring() + L"\\";
	{
		// path.parent_path().generic_wstring() 
		aiString tempName;
		material.Get(AI_MATKEY_NAME, tempName);
		m_Name = tempName.C_Str();
	}
	
	aiString DiffuseFileName;
	aiString SpecularFileName;
	aiString NormalFileName;

	BOOL bHasTexture = false;
	BOOL bHasGlossAlpha = false;

	{
		Technique PrePass("PrePass");

		Step ShadowPrePass("ShadowPrePass");

		PrePass.PushBackStep(ShadowPrePass);
		m_Techniques.push_back(std::move(PrePass));
	}

	{
		static auto pfnPushBackTexture = [](std::basic_string<wchar_t> path, ePixelLayoutFlags flag, Step& _Step)
		{
			const ManagedTexture* tempTexture = TextureManager::LoadWICFromFile(path);

			std::lock_guard<std::mutex> LockGuard(sm_mutex);

			_Step.PushBack(std::make_shared<ManagedTexture>(tempTexture));
		};

		Technique Phong("Phong", eObjectFilterFlag::kOpaque);
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

					pfnPushBackTexture(rootPath + AnsiToWString(DiffuseFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_DIFFUSE_TEXTURE_F2, Lambertian);
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

					pfnPushBackTexture(rootPath + AnsiToWString(SpecularFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_SPEC_TEXTURE_F2, Lambertian);
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

					pfnPushBackTexture(rootPath + AnsiToWString(NormalFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_NORMAL_TEXTURE_F2, Lambertian);
				}
			}
		} // End Select out Material

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
				unsigned long kSpecularFlag{ (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_GLOSS_ALPHA_B1 }; // Has Specular?
				if (_BitScanForward(&kSpecularFlag, m_PixelInputLayoutFlag))
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
				unsigned long kNormalFlag{ (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_NORMAL_B1 }; // Has Normal?
				if (_BitScanForward(&kNormalFlag, m_PixelInputLayoutFlag))
				{
					BOOL* m_UseNormalMap = &m_CustomBuffer.m_Data.UseNormalMap;
					*m_UseNormalMap = true;

					float* m_pNormalMapWeight = &m_CustomBuffer.m_Data.NormalMapWeight;
					*m_pNormalMapWeight = 1.0f;
				}
			}

			Lambertian.PushBack(std::make_shared<CustomBuffer>(m_CustomBuffer));

			
			// std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
			// InputElements.reserve(5);

			m_InputElements.reserve(5);

			// Set InputElements
			{
				size_t elementsIndex{ 0 };

				for (unsigned long index{ 1 }; index < (unsigned long)eVertexLayoutFlags::ATTRIB_MASK_COUNT; index <<= 1, ++elementsIndex)
				{
					if (_BitScanForward(&index, m_VertexInputLayoutFlag))
					{
						// InputElements.push_back(premade::g_InputElements[elementsIndex]);
						m_InputElements.push_back(premade::g_InputElements[elementsIndex]);
					}
				}
			}
			
			Phong.PushBackStep(Lambertian);
		} // End Set CustomBuffer

		m_Techniques.push_back(std::move(Phong));
	}


}


/*
Material::Material(aiMaterial& material, const std::filesystem::path& path) DEBUG_EXCEPT
	: m_ModelFilePath(path.string())
{
	// const std::basic_string rootPath = path.parent_path().string() + "\\";
	// path.parent_path().generic_wstring()
	const std::basic_string<wchar_t> rootPath = path.parent_path().generic_wstring() + L"\\";
	{
		// path.parent_path().generic_wstring()
		aiString tempName;
		material.Get(AI_MATKEY_NAME, tempName);
		m_Name = tempName.C_Str();
	}

	auto pfnPushBackTexture = [](std::basic_string<wchar_t> path, ePixelLayoutFlags flag, std::vector<ManagedTexture*> container, Step _Step)
	{
		ManagedTexture tempTexture = ManagedTexture((path), (uint32_t)(flag));

		std::lock_guard<std::mutex> LockGuard(sm_mutex);

		_Step.PushBack(std::make_shared<ManagedTexture>(tempTexture)); // step.AddBindable(std::move(tex));
		container.push_back(&tempTexture);
	};

	uint32_t MaterialCount{ 0 };

	std::vector<ManagedTexture*> ManagedTextures;
	ManagedTextures.clear();

	{
		Step step("ShadowPrePass");
		Technique RenderLightShadows("RenderLightShadows", eObjectFilterFlag::kOpaque);

	}

	// Phong technique
	{
		Step step("Lambertian");
		Technique phong{ "Phong", eObjectFilterFlag::kOpaque };
		aiString textureFileName;

		m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_POSITION_F3; // VertexLayout
		m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_NORMAL_F3;   // VertexLayout

		BOOL bHasTexture = false;
		BOOL bHasGlossAlpha = false;

		// Diffuse
		{
			if (material.GetTexture(aiTextureType_DIFFUSE, 0, &textureFileName) == aiReturn_SUCCESS)
			{
				bHasTexture = true;

				m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_TEXCOORD_F2; // VertexLayout

				// ManagedTexture tempTexture = ManagedTexture(rootPath + AnsiToWString(textureFileName.C_Str()), (uint32_t)eCompositionFlags::ATTRIB_MASK_TEXCOORD2);
				// step.PushBack(std::make_shared<ManagedTexture>(tempTexture)); // step.AddBindable(std::move(tex));
				// ManagedTextures.emplace_back(tempTexture);
				pfnPushBackTexture(rootPath + AnsiToWString(textureFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_DIFFUSE_TEXTURE_F2, ManagedTextures, step);
			}
			else
			{
				m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_MATERIAL_COLOR_F3; // PixelLayout
			}
			// PhongPSO = premade::g_RasterizerTwoSided;
		}

		// Specular
		{
			if (material.GetTexture(aiTextureType_SPECULAR, 0, &textureFileName) == aiReturn_SUCCESS)
			{
				bHasTexture = true;
				m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_TEXCOORD_F2;       // VertexLayout

				m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_GLOSS_ALPHA_B1; // PixelLayout
				m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_MAP_B1;         // PixelLayout

				// ManagedTexture tempTexture = ManagedTexture(rootPath + AnsiToWString(textureFileName.C_Str()), (uint32_t)eCompositionFlags::ATTRIB_MASK_SPCTEXCOORD2);
				// step.PushBack(std::make_shared<ManagedTexture>(tempTexture)); // step.AddBindable(std::move(tex));
				// ManagedTextures.emplace_back(tempTexture);
				pfnPushBackTexture(rootPath + AnsiToWString(textureFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_SPEC_TEXTURE_F2, ManagedTextures, step);
			}
			// pscLayout.Add<Dcb::Float3>("specularColor");
			// pscLayout.Add<Dcb::Float>("specularWeight");
			// pscLayout.Add<Dcb::Float>("specularGloss");
			m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_COLOR_F3;   // PixelLayout
			m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_WEIGHT_F1;  // PixelLayout
			m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_GLOSS_F1;   // PixelLayout
		}

		// normal
		{
			if (material.GetTexture(aiTextureType_NORMALS, 0, &textureFileName) == aiReturn_SUCCESS)
			{
				m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_TEXCOORD_F2;           // VertexLayout
				m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_NORMAL_TANGENT_F3;     // VertexLayout
				m_VertexInputLayoutFlag |= (uint32_t)eVertexLayoutFlags::ATTRIB_MASK_NORMAL_BITANGENT_F3;   // VertexLayout

				m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_NORMAL_B1;               // PixelLayout  // RootConstant
				m_PixelInputLayoutFlag |= (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_NORMAL_WEIGHT_F1;        // PixelLayout  // RootConstant

				// ManagedTexture tempTexture = ManagedTexture(rootPath + AnsiToWString(textureFileName.C_Str()), (uint32_t)eCompositionFlags::ATTRIB_MASK_SPCTEXCOORD2);
				// step.PushBack(std::make_shared<ManagedTexture>(tempTexture)); // step.AddBindable(std::move(tex));
				// ManagedTextures.emplace_back(tempTexture);
				pfnPushBackTexture(rootPath + AnsiToWString(textureFileName.C_Str()), ePixelLayoutFlags::ATTRIB_MASK_NORMAL_TEXTURE_F2, ManagedTextures, step);
			}
		}

		// Common (post)
		{
			custom::RootSignature m_RootSignature; // m_RootSignature.Reset(4, (bHasTexture? 2 : 1));

			m_RootSignature.Reset(4, 2);
			m_RootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_VERTEX);
			m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_PIXEL);
			m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
			m_RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL);

			custom::SamplerDescriptor DefaultSamplerDesc;
			DefaultSamplerDesc.MaxAnisotropy = 8;
			m_RootSignature.InitStaticSampler(0, DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
			m_RootSignature.InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
			m_RootSignature.Finalize(L"ModelRS " + StringToWString(m_ModelFilePath), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			step.PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));

			std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
			InputElements.reserve(5);

			// Set InputElements
			{
				size_t elementsIndex{ 0 };

				for (unsigned long index{ 1 }; index < (unsigned long)eVertexLayoutFlags::ATTRIB_MASK_COUNT; index <<= 1, ++elementsIndex)
				{
					if (_BitScanForward(&index, m_VertexInputLayoutFlag))
					{
						InputElements.push_back(premade::g_InputElements[elementsIndex]);
					}
				}
			}

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
					unsigned long kSpecularFlag{ (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_SPEC_GLOSS_ALPHA_B1 };
					if (_BitScanForward(&kSpecularFlag, m_PixelInputLayoutFlag))
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
					unsigned long kNormalFlag{ (uint32_t)ePixelLayoutFlags::ATTRIB_MASK_NORMAL_B1 };
					if (_BitScanForward(&kNormalFlag, m_PixelInputLayoutFlag))
					{
						BOOL* m_UseNormalMap = &m_CustomBuffer.m_Data.UseNormalMap;
						*m_UseNormalMap = true;

						float* m_pNormalMapWeight = &m_CustomBuffer.m_Data.NormalMapWeight;
						*m_pNormalMapWeight = 1.0f;
					}
				}

				step.PushBack(std::make_shared<CustomBuffer>(GetCustomBuffer()));
			} // End Set CustomBuffer

			GraphicsPSO PhongPSO;

			PhongPSO.SetRootSignature(m_RootSignature);
			PhongPSO.SetRasterizerState(premade::g_RasterizerTwoSided); // PhongPSO = premade::g_RasterizerTwoSided;
			PhongPSO.SetBlendState(premade::g_BlendDisable);
			PhongPSO.SetDepthStencilState(premade::g_DepthStateTestEqual);
			PhongPSO.SetInputLayout(InputElements.size(), InputElements.data());
			PhongPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			PhongPSO.SetVertexShader(g_pGeneralPurposeVS, sizeof(g_pGeneralPurposeVS));
			PhongPSO.SetPixelShader(g_pGeneralPurposePS, sizeof(g_pGeneralPurposePS));
			{
				DXGI_FORMAT RTVFormat = bufferManager::g_SceneColorBuffer.GetFormat();
				DXGI_FORMAT DSVFormat = bufferManager::g_SceneDepthBuffer.GetFormat();
				PhongPSO.SetRenderTargetFormats(1, &RTVFormat, DSVFormat);
			}
			PhongPSO.Finalize();
			step.PushBack(std::make_shared<GraphicsPSO>(PhongPSO));

			phong.PushBackStep(step);
			m_Techniques.push_back(phong);
		} // Common (post)
	} // Phong technique

	static BOOL bIsDone{ false };
	Technique outline{ "Outline Masking", eObjectFilterFlag::kOpaque };
	Technique map{ "ShadowMap", eObjectFilterFlag::kOpaque };


}

*/