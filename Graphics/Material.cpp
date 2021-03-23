#include "stdafx.h"
#include "Material.h"

#include <assimp/material.h>
#include <assimp/types.h>

#include "RootSignature.h"
#include "Device.h"
#include "PreMadePSO.h"
#include "BufferManager.h"
#include "SamplerDescriptor.h"
#include "ManagedTexture.h"
#include "FundamentalTexture.h"
#include "TextureManager.h"
#include "RenderingResource.h"
#include "MaterialConstants.h"
#include "TransformConstants.h"
#include "ColorConstants.h"
#include "RawPSO.h"
#include "ObjectFilterFlag.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_NONE_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_Diffuse_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_Specular_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_Normal_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_DiffuseSpecular_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_DiffuseNormal_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_SpecularNormal_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelTest_DiffuseSpecularNormal_PS.h"

#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"

#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_VS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_NONE_PS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_Diffuse_PS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_Specular_PS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_Normal_PS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_DiffuseSpecular_PS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_DiffuseNormal_PS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_SpecularNormal_PS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelTest_DiffuseSpecularNormal_PS.h"

#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"

#endif

// WaveFront Objfile Syntax
// 
// Ns - Shininess of the _aiMaterial (exponent).
// sharpness - sharpness fo the reflections for the _aiMaterial. 
//           - Default value is 60.
// Ni - Optical Density for surface(a.k.a index of refraction).
//      Glass has 1.5.
// Ka - Color of ambient is multiplied by the texture value.
// Kd - Color of diffuse  ""
// Ks - Color of specular ""
// Ns - Material Specular exponent is multiplied by texture value.
// d  - Non-transparency of the meterial alpha.
// Tr - Transparency of the _aiMaterial alpha.
//  Illumination    Properties that are turned on in the Model
// 0		Color on and Ambient off
// 1		Color on and Ambient on
// 2		Highlight on
// 3		Reflection on and Ray trace on
// 4		Transparency: Glass on
// Reflection : Ray trace on
// 5		Reflection : Fresnel on and Ray trace on
// 6		Transparency : Refraction on
// Reflection : Fresnel off and Ray trace on
// 7		Transparency : Refraction on
// Reflection : Fresnel on and Ray trace on
// 8		Reflection on and Ray trace off
// 9		Transparency : Glass on
// Reflection : Ray trace off
// 10		Casts shadows onto invisible surfaces
/*
#define _AI_MATKEY_TEXTURE_BASE         "$tex.file"
#define _AI_MATKEY_UVWSRC_BASE          "$tex.uvwsrc"
#define _AI_MATKEY_TEXOP_BASE           "$tex.op"
#define _AI_MATKEY_MAPPING_BASE         "$tex.mapping"
#define _AI_MATKEY_TEXBLEND_BASE        "$tex.blend"
#define _AI_MATKEY_MAPPINGMODE_U_BASE   "$tex.mapmodeu"
#define _AI_MATKEY_MAPPINGMODE_V_BASE   "$tex.mapmodev"
#define _AI_MATKEY_TEXMAP_AXIS_BASE     "$tex.mapaxis"
#define _AI_MATKEY_UVTRANSFORM_BASE     "$tex.uvtrafo"
#define _AI_MATKEY_TEXFLAGS_BASE        "$tex.flags"
//! @endcond

// ---------------------------------------------------------------------------
#define AI_MATKEY_TEXTURE(type, N) _AI_MATKEY_TEXTURE_BASE,type,N

// For backward compatibility and simplicity
//! @cond MATS_DOC_FULL
#define AI_MATKEY_TEXTURE_DIFFUSE(N)    \
	AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE,N)

#define AI_MATKEY_TEXTURE_SPECULAR(N)   \
	AI_MATKEY_TEXTURE(aiTextureType_SPECULAR,N)

#define AI_MATKEY_TEXTURE_AMBIENT(N)    \
	AI_MATKEY_TEXTURE(aiTextureType_AMBIENT,N)

#define AI_MATKEY_TEXTURE_EMISSIVE(N)   \
	AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE,N)

#define AI_MATKEY_TEXTURE_NORMALS(N)    \
	AI_MATKEY_TEXTURE(aiTextureType_NORMALS,N)

#define AI_MATKEY_TEXTURE_HEIGHT(N) \
	AI_MATKEY_TEXTURE(aiTextureType_HEIGHT,N)

#define AI_MATKEY_TEXTURE_SHININESS(N)  \
	AI_MATKEY_TEXTURE(aiTextureType_SHININESS,N)

#define AI_MATKEY_TEXTURE_OPACITY(N)    \
	AI_MATKEY_TEXTURE(aiTextureType_OPACITY,N)

#define AI_MATKEY_TEXTURE_DISPLACEMENT(N)   \
	AI_MATKEY_TEXTURE(aiTextureType_DISPLACEMENT,N)

#define AI_MATKEY_TEXTURE_LIGHTMAP(N)   \
	AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP,N)

#define AI_MATKEY_TEXTURE_REFLECTION(N) \
	AI_MATKEY_TEXTURE(aiTextureType_REFLECTION,N)
*/

#define TransformRootIndex 1u
#define Color3RootIndex    3u
#define MaterialRootIndex  4u

Material::Material(aiMaterial& _aiMaterial, const std::filesystem::path& path) DEBUG_EXCEPT
	: m_ModelFilePath(path.string())
{
	const std::basic_string<char> rootPath = path.parent_path().string() + "\\";
	{
		aiString tempName;
		ASSERT(_aiMaterial.Get(AI_MATKEY_NAME, tempName) == aiReturn_SUCCESS);
		m_Name = tempName.C_Str();
	}

	static auto pfnPushBackTexture = [](std::basic_string<char> path, Step& _Step, UINT RootIndex, UINT Offset)
	{
		const ManagedTexture* tempTexture = TextureManager::LoadWICFromFile(AnsiToWString(path.c_str()), RootIndex, Offset);

		FundamentalTexture temp(path.c_str(), RootIndex, Offset, tempTexture->GetSRV());

		// æµ≤®∏È _Step¿« Private CriticalSection
		_Step.PushBack(std::make_shared<FundamentalTexture>(temp));
	};

	aiString DiffuseFileName;
	aiString SpecularFileName;
	aiString EmessiveFileName; // Not Used.
	aiString NormalFileName;
	aiString LightMapFileName; // Not Used.
	aiString ReflectionFileName; // Not Used.

	BOOL bHasTexture = false;
	BOOL bHasGlossAlpha = false;

	// Technique PrePass("PrePass");
	Technique __ShadowPrePass("ShadowPrePass");
	Step _ShadowPrePass("Shadowing");

	Technique __Z_PrePass("Z_PrePass");
	Step _Z_PrePass("Z_Calculating");

	Technique _ShadowPass("ShadowMappingPass");
	Step _ShadowMappingPass("ShadowMapping");

	_ShadowPrePass.PushBack(std::make_shared<TransformConstants>(TransformRootIndex));
	_Z_PrePass.PushBack(std::make_shared<TransformConstants>(TransformRootIndex));
	_ShadowMappingPass.PushBack(std::make_shared<TransformConstants>(TransformRootIndex));

	__ShadowPrePass.PushBackStep(std::move(_ShadowPrePass));
	__Z_PrePass.PushBackStep(std::move(_Z_PrePass));
	_ShadowPass.PushBackStep(std::move(_ShadowMappingPass));

	m_Techniques.push_back(std::move(__ShadowPrePass));
	m_Techniques.push_back(std::move(__Z_PrePass));
	m_Techniques.push_back(std::move(_ShadowPass));

	DXGI_FORMAT ColorFormat   = bufferManager::g_SceneColorBuffer.GetFormat();
	DXGI_FORMAT StencilFormat = bufferManager::g_OutlineBuffer.GetFormat();
	DXGI_FORMAT DepthFormat   = bufferManager::g_SceneDepthBuffer.GetFormat();

	{
		Technique Phong("MainRenderPass");
		Step Lambertian("Phong");

		unsigned long TextureBitMap = 0;

		// Select out Material
		{
			// Diffuse
			{
				if (_aiMaterial.GetTexture(aiTextureType_DIFFUSE, 0, &DiffuseFileName) == aiReturn_SUCCESS)
				{
					// pfnPushBackTexture(rootPath + DiffuseFileName.C_Str(), Lambertian, 3u, 0u);
					TextureBitMap |= 1;
				}
				else
				{
				}
			}

			// Specular
			{
				if (_aiMaterial.GetTexture(aiTextureType_SPECULAR, 0, &SpecularFileName) == aiReturn_SUCCESS)
				{
					// pfnPushBackTexture(rootPath + SpecularFileName.C_Str(), Lambertian, 3u, 1u);
					TextureBitMap |= 2;
				}
				else
				{
				}
			}

			// normal
			{
				if (_aiMaterial.GetTexture(aiTextureType_NORMALS, 0, &NormalFileName) == aiReturn_SUCCESS)
				{
					// pfnPushBackTexture(rootPath + NormalFileName.C_Str(), Lambertian, 3u, 3u);
					TextureBitMap |= 4;
				}
				else
				{
				}
			}

		} // End Select out Material

		Lambertian.PushBack(std::make_shared<TransformConstants>(TransformRootIndex));

		custom::RootSignature MainRootSignature;
		MainRootSignature.Reset(6, 2);
		MainRootSignature.InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		MainRootSignature.InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		MainRootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)
		MainRootSignature[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b1)
		MainRootSignature[2].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)
		MainRootSignature[3].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b1)
		MainRootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
		// Texture2D<float3> texDiffuse     : register(t0);
		// Texture2D<float3> texSpecular    : register(t1);
		// Texture2D<float4> texEmissive    : register(t2);
		// Texture2D<float3> texNormal      : register(t3);
		// Texture2D<float4> texLightmap    : register(t4);
		// Texture2D<float4> texReflection  : register(t5);
		MainRootSignature[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL);
		// Texture2D<float>            texSSAO             : register(t64);
		// Texture2D<float>            texShadow           : register(t65);
		// StructuredBuffer<LightData> lightBuffer         : register(t66);
		// Texture2DArray<float>       lightShadowArrayTex : register(t67);
		// ByteAddressBuffer           lightGrid           : register(t68);
		// ByteAddressBuffer           lightGridBitMask    : register(t69);
		MainRootSignature.Finalize(L"MainRS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Lambertian.PushBack(std::make_shared<custom::RootSignature>(MainRootSignature));

		GraphicsPSO MainPSO;
		MainPSO.SetRootSignature(MainRootSignature);
		MainPSO.SetRasterizerState(premade::g_RasterizerDefault);
		// MainPSO.SetBlendState           (premade::g_BlendNoColorWrite);
		// MainPSO.SetDepthStencilState    (premade::g_DepthStateReadWrite);
		// MainPSO.SetRenderTargetFormats  (0, nullptr, DepthFormat);
		MainPSO.SetInputLayout(5, premade::g_InputElements);
		MainPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		MainPSO.SetBlendState(premade::g_BlendDisable);
		MainPSO.SetDepthStencilState(premade::g_DepthStateTestEqual);
		MainPSO.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
		MainPSO.SetVertexShader(g_pModelTest_VS, sizeof(g_pModelTest_VS));

		aiColor4D TempDiffuseColor;
		aiColor4D TempSpecularColor;
		float TempSpecularGloss;
		std::wstring PSOWName;

		switch (TextureBitMap)
		{
		case 0:
			MainPSO.SetPixelShader(g_pModelTest_None_PS, sizeof(g_pModelTest_None_PS));
			PSOWName += L"NoneTextureMap-";
			PSOWName += AnsiToWString(m_Name.c_str());
			MainPSO.Finalize(PSOWName.c_str());
			Lambertian.PushBack(std::make_shared<GraphicsPSO>(MainPSO));

			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, TempDiffuseColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_SPECULAR, TempSpecularColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_SHININESS, TempSpecularGloss));
			Lambertian.PushBack(std::make_shared<ControllerConstants0>(*reinterpret_cast<Math::Vector3*>(&TempDiffuseColor), *reinterpret_cast<Math::Vector3*>(&TempSpecularColor), TempSpecularGloss));
			break;
		case 1:
			MainPSO.SetPixelShader(g_pModelTest_Diffuse_PS, sizeof(g_pModelTest_Diffuse_PS));
			PSOWName += L"DiffuseMap-";
			PSOWName += AnsiToWString(m_Name.c_str());
			MainPSO.Finalize(PSOWName.c_str());
			Lambertian.PushBack(std::make_shared<GraphicsPSO>(MainPSO));

			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_SPECULAR, TempSpecularColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_SHININESS, TempSpecularGloss));
			pfnPushBackTexture(rootPath + DiffuseFileName.C_Str(), Lambertian, MaterialRootIndex, 0u);
			Lambertian.PushBack(std::make_shared<ControllerConstants1>(*reinterpret_cast<Math::Vector3*>(&TempSpecularColor), TempSpecularGloss));
			break;
		case 2:
			MainPSO.SetPixelShader(g_pModelTest_Specular_PS, sizeof(g_pModelTest_Specular_PS));
			PSOWName += L"SpecularMap-";
			PSOWName += AnsiToWString(m_Name.c_str());
			MainPSO.Finalize(PSOWName.c_str());
			Lambertian.PushBack(std::make_shared<GraphicsPSO>(MainPSO));

			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, TempDiffuseColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_SPECULAR, TempSpecularColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_SHININESS, TempSpecularGloss));
			pfnPushBackTexture(rootPath + SpecularFileName.C_Str(), Lambertian, MaterialRootIndex, 1u);
			Lambertian.PushBack(std::make_shared<ControllerConstants2>(*reinterpret_cast<Math::Vector3*>(&TempDiffuseColor), *reinterpret_cast<Math::Vector3*>(&TempSpecularColor), TempSpecularGloss));
			break;
		case 3:
			MainPSO.SetPixelShader(g_pModelTest_DiffuseSpecular_PS, sizeof(g_pModelTest_DiffuseSpecular_PS));
			PSOWName += L"DiffuseSpecularMap-";
			PSOWName += AnsiToWString(m_Name.c_str());
			MainPSO.Finalize(PSOWName.c_str());
			Lambertian.PushBack(std::make_shared<GraphicsPSO>(MainPSO));

			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_SPECULAR, TempSpecularColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_SHININESS, TempSpecularGloss));
			pfnPushBackTexture(rootPath + DiffuseFileName.C_Str(), Lambertian, MaterialRootIndex, 0u);
			pfnPushBackTexture(rootPath + SpecularFileName.C_Str(), Lambertian, MaterialRootIndex, 1u);
			Lambertian.PushBack(std::make_shared<ControllerConstants3>(*reinterpret_cast<Math::Vector3*>(&TempSpecularColor), TempSpecularGloss));
			break;
		case 4:
			MainPSO.SetPixelShader(g_pModelTest_Normal_PS, sizeof(g_pModelTest_Normal_PS));
			PSOWName += L"NormalMap-";
			PSOWName += AnsiToWString(m_Name.c_str());
			MainPSO.Finalize(PSOWName.c_str());
			Lambertian.PushBack(std::make_shared<GraphicsPSO>(MainPSO));

			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, TempDiffuseColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_SPECULAR, TempSpecularColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_SHININESS, TempSpecularGloss));
			pfnPushBackTexture(rootPath + NormalFileName.C_Str(), Lambertian, MaterialRootIndex, 3u);
			Lambertian.PushBack(std::make_shared<ControllerConstants4>(*reinterpret_cast<Math::Vector3*>(&TempDiffuseColor), *reinterpret_cast<Math::Vector3*>(&TempSpecularColor), TempSpecularGloss));
			break;
		case 5:
			MainPSO.SetPixelShader(g_pModelTest_DiffuseNormal_PS, sizeof(g_pModelTest_DiffuseNormal_PS));
			PSOWName += L"DiffuseNormalMap-";
			PSOWName += AnsiToWString(m_Name.c_str());
			MainPSO.Finalize(PSOWName.c_str());
			Lambertian.PushBack(std::make_shared<GraphicsPSO>(MainPSO));

			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_SPECULAR, TempSpecularColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_SHININESS, TempSpecularGloss));
			pfnPushBackTexture(rootPath + DiffuseFileName.C_Str(), Lambertian, MaterialRootIndex, 0u);
			pfnPushBackTexture(rootPath + NormalFileName.C_Str(), Lambertian, MaterialRootIndex, 3u);
			Lambertian.PushBack(std::make_shared<ControllerConstants5>(*reinterpret_cast<Math::Vector3*>(&TempSpecularColor), TempSpecularGloss));
			break;
		case 6:
			MainPSO.SetPixelShader(g_pModelTest_SpecularNormal_PS, sizeof(g_pModelTest_SpecularNormal_PS));
			PSOWName += L"SpecularNormalMap-";
			PSOWName += AnsiToWString(m_Name.c_str());
			MainPSO.Finalize(PSOWName.c_str());
			Lambertian.PushBack(std::make_shared<GraphicsPSO>(MainPSO));

			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, TempDiffuseColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_SPECULAR, TempSpecularColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_SHININESS, TempSpecularGloss));
			pfnPushBackTexture(rootPath + SpecularFileName.C_Str(), Lambertian, MaterialRootIndex, 1u);
			pfnPushBackTexture(rootPath + NormalFileName.C_Str(), Lambertian, MaterialRootIndex, 3u);
			Lambertian.PushBack(std::make_shared<ControllerConstants6>(*reinterpret_cast<Math::Vector3*>(&TempDiffuseColor), *reinterpret_cast<Math::Vector3*>(&TempSpecularColor), TempSpecularGloss));
			break;
		case 7:
			MainPSO.SetPixelShader(g_pModelTest_DiffuseSpecularNormal_PS, sizeof(g_pModelTest_DiffuseSpecularNormal_PS));
			PSOWName += L"DiffuseSpecularNormalMap-";
			PSOWName += AnsiToWString(m_Name.c_str());
			MainPSO.Finalize(PSOWName.c_str());
			Lambertian.PushBack(std::make_shared<GraphicsPSO>(MainPSO));

			ASSERT(!_aiMaterial.Get(AI_MATKEY_COLOR_SPECULAR, TempSpecularColor));
			ASSERT(!_aiMaterial.Get(AI_MATKEY_SHININESS, TempSpecularGloss));
			pfnPushBackTexture(rootPath + DiffuseFileName.C_Str(), Lambertian, MaterialRootIndex, 0u);
			pfnPushBackTexture(rootPath + SpecularFileName.C_Str(), Lambertian, MaterialRootIndex, 1u);
			pfnPushBackTexture(rootPath + NormalFileName.C_Str(), Lambertian, MaterialRootIndex, 3u);
			Lambertian.PushBack(std::make_shared<ControllerConstants7>(*reinterpret_cast<Math::Vector3*>(&TempSpecularColor), TempSpecularGloss));
			break;
		default:
			ASSERT(false, "Invalid TextureBitMap.");
		}

		Phong.PushBackStep(std::move(Lambertian));
		m_Techniques.push_back(std::move(Phong));
	}

	{
		Technique Outline("OutlineDrawingPass", eObjectFilterFlag::kOpaque, false);
		Step OutlineMaskStep(" OutlineMasking");
		Step OutlineDraw("OutlineDraw");

		custom::RootSignature OutlineRootSignature;
		OutlineRootSignature.Reset(6, 2);
		OutlineRootSignature.InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		OutlineRootSignature.InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		OutlineRootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)
		OutlineRootSignature[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b1)
		OutlineRootSignature[2].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)
		OutlineRootSignature[3].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b1)
		OutlineRootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
		OutlineRootSignature[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL);
		OutlineRootSignature.Finalize(L"OutlineRS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		OutlineMaskStep.PushBack(std::make_shared<custom::RootSignature>(OutlineRootSignature));
		OutlineDraw.PushBack(std::make_shared<custom::RootSignature>(OutlineRootSignature));

		{
			GraphicsPSO OutlineMaskPSO;
			OutlineMaskPSO.SetRootSignature(OutlineRootSignature);
			OutlineMaskPSO.SetRasterizerState(premade::g_RasterizerBackSided);
			OutlineMaskPSO.SetDepthStencilState(premade::g_DepthStencilWrite);
			OutlineMaskPSO.SetRenderTargetFormats(0, nullptr, DepthFormat);
			OutlineMaskPSO.SetBlendState(premade::g_BlendDisable);
			OutlineMaskPSO.SetInputLayout(5, premade::g_InputElements);
			OutlineMaskPSO.SetVertexShader(g_pFlat_VS, sizeof(g_pFlat_VS));
			OutlineMaskPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			OutlineMaskPSO.Finalize(L"OutlineMaskPSO");

			OutlineMaskStep.PushBack(std::make_shared<GraphicsPSO>(OutlineMaskPSO));
			OutlineMaskStep.PushBack(std::make_shared<TransformConstants>(TransformRootIndex));

			Outline.PushBackStep(OutlineMaskStep);
		}
		{
			GraphicsPSO OutlineDrawPSO;
			OutlineDrawPSO.SetRootSignature(OutlineRootSignature);
			OutlineDrawPSO.SetRasterizerState(premade::g_RasterizerBackSided);
			OutlineDrawPSO.SetDepthStencilState(premade::g_DepthStencilMask);
			OutlineDrawPSO.SetRenderTargetFormat(StencilFormat, DXGI_FORMAT_UNKNOWN);
			OutlineDrawPSO.SetBlendState(premade::g_BlendOutlineDrawing);
			OutlineDrawPSO.SetInputLayout(5, premade::g_InputElements);
			OutlineDrawPSO.SetVertexShader(g_pFlat_VS, sizeof(g_pFlat_VS));
			OutlineDrawPSO.SetPixelShader(g_pFlat_PS, sizeof(g_pFlat_PS));
			OutlineDrawPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			OutlineDrawPSO.Finalize(L"OutlineDrawPSO");

			OutlineDraw.PushBack(std::make_shared<GraphicsPSO>(OutlineDrawPSO));
			// OutlineDraw.PushBack(std::make_shared<TransformConstants>(TransformRootIndex));

			// TODO : ColorConstants
			// Color3Buffer ColorBuffer;
			// OutlineDraw.PushBack(std::make_shared<Color3Buffer>(ColorBuffer));
			OutlineDraw.PushBack(std::make_shared<Color3Buffer>(Color3RootIndex));

			Outline.PushBackStep(OutlineDraw);
		}
		m_Techniques.push_back(std::move(Outline));
	}

	// TestPass
	{
		Technique Debug_Technique("DebugWireFramePass");
		Step Debug_Step("Debug");
		Debug_Technique.PushBackStep(std::move(Debug_Step));
		m_Techniques.push_back(std::move(Debug_Technique));
	}
}