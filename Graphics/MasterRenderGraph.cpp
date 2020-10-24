#include "stdafx.h"
#include "MasterRenderGraph.h"

#include "ShadowBuffer.h"
#include "BufferManager.h"
#include "PreMadePSO.h"
#include "ObjectFilterFlag.h"
#include "Matrix4.h"
#include "ShaderConstantsTypeDefinitions.h"

#include "Camera.h"
#include "RenderTarget.h"
#include "DepthStencil.h"

#include "Pass.h"
#include "LightPrePass.h"
#include "ShadowPrePass.h"
#include "Z_PrePass.h"
#include "SSAOPass.h"
#include "FillLightGridPass.h"
#include "ShadowMappingPass.h"
#include "MainRenderPass.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthPS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_Basic.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_TexCoord.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_TexNormal.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_Basic.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_TexCoord.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_TexNormal.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h" // For OutlineDrawingShader (Current State : Pseudo)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h" // For OutlineDrawingShader (Current State : Pseudo)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/OutlineWithBlurPS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/HoriontalBlurCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/VerticalBlurCS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Relase/Graphics(.lib)/CompiledShaders/DepthPS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_Basic.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_TexCoord.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelVS_TexNormal.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_Basic.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_TexCoord.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ModelPS_TexNormal.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_VS.h" // For OutlineDrawingShader (Current State : Pseudo)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_PS.h" // For OutlineDrawingShader (Current State : Pseudo)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/OutlineWithBlurPS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/HoriontalBlurCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/VerticalBlurCS.h"
#endif

// TODO :Pass들 세팅
// https://developer.nvidia.com/sites/all/modules/custom/gpugems/books/GPUGems/gpugems_ch12.html
// https://docs.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps
// https://www.gamedev.net/forums/topic/657728-standard-approach-to-shadow-mapping-multiple-light-sources/
// http://www.adriancourreges.com/blog/2016/09/09/doom-2016-graphics-study/
// http://ivizlab.sfu.ca/papers/cgf2012.pdf
// https://www.slideshare.net/blindrenderer/rendering-tech-of-space-marinekgc-2011?next_slideshow=1

MasterRenderGraph::MasterRenderGraph()
//: m_pRenderTarget(), m_pMasterDepth()
{
	// TODO : RootSignature, PSO 세팅
	{
		m_RootSignature;
		m_RootSignature.Reset(4, 2);
		m_RootSignature.InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);              //
		m_RootSignature.InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);               //
		m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)                   //
		m_RootSignature[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)                   //
		m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);  //
		// Texture2D<float3> texDiffuse     : register(t0);
	    // Texture2D<float3> texSpecular    : register(t1);
	    // Texture2D<float4> texEmissive    : register(t2);
	    // Texture2D<float3> texNormal      : register(t3);
	    // Texture2D<float4> texLightmap    : register(t4);
	    // Texture2D<float4> texReflection  : register(t5);

		m_RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL);
		// Texture2D<float>            texSSAO             : register(t64);
		// Texture2D<float>            texShadow           : register(t65);
		// StructuredBuffer<LightData> lightBuffer         : register(t66);
		// Texture2DArray<float>       lightShadowArrayTex : register(t67);
		// ByteAddressBuffer           lightGrid           : register(t68);
		// ByteAddressBuffer           lightGridBitMask    : register(t69);

		m_RootSignature.Finalize(L"MainRS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}
	DXGI_FORMAT ColorFormat = bufferManager::g_SceneColorBuffer.GetFormat();
	DXGI_FORMAT DepthFormat = bufferManager::g_SceneDepthBuffer.GetFormat();

	D3D12_INPUT_ELEMENT_DESC InputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	// Depth Shader
	{
		m_DepthPSO.SetRootSignature(m_RootSignature);
		m_DepthPSO.SetRasterizerState(premade::g_RasterizerDefault);
		m_DepthPSO.SetBlendState(premade::g_BlendNoColorWrite);
		m_DepthPSO.SetDepthStencilState(premade::g_DepthStateReadWrite);
		m_DepthPSO.SetInputLayout(_countof(InputElements), InputElements);
		m_DepthPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_DepthPSO.SetRenderTargetFormats(0, nullptr, DepthFormat);
		m_DepthPSO.SetVertexShader(g_pDepthVS, sizeof(g_pDepthVS));
		m_DepthPSO.Finalize();
	}
	// Shadow Shader
	{
		m_ShadowPSO = m_DepthPSO;
		m_ShadowPSO.SetRasterizerState(premade::g_RasterizerShadow);
		m_ShadowPSO.SetRenderTargetFormats(0, nullptr, bufferManager::g_ShadowBuffer.GetFormat());
		m_ShadowPSO.Finalize();
	}
	// MainPass Shader
	{
		m_MainRenderPSO = m_DepthPSO;
		m_MainRenderPSO.SetBlendState(premade::g_BlendDisable);
		m_MainRenderPSO.SetDepthStencilState(premade::g_DepthStateTestEqual);
		m_MainRenderPSO.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
		m_MainRenderPSO.SetVertexShader(g_pModelVS_TexNormal, sizeof(g_pModelVS_TexNormal));
		m_MainRenderPSO.SetPixelShader(g_pModelPS_TexNormal, sizeof(g_pModelPS_TexNormal));
		m_MainRenderPSO.Finalize();
	}

	// *1* LightingPrePass
	{
		std::unique_ptr<LightPrePass> _LightPrePass = std::make_unique<LightPrePass>("LightPrePass");
		appendPass(std::move(_LightPrePass));		
		// m_pLightPrePass = (LightPrePass*)m_pPasses.back().get();
		m_pLightPrePass = dynamic_cast<LightPrePass*>(m_pPasses.back().get());
		ASSERT(m_pLightPrePass);
	}
	// *2* ShadowPrePass
	{
		std::unique_ptr<ShadowPrePass> _ShadowPrePass = std::make_unique<ShadowPrePass>("ShadowPrePass", &m_RootSignature);
		appendPass(std::move(_ShadowPrePass));
		// m_pShadowPrePass = (ShadowPrePass*)m_pPasses.back().get();
		m_pShadowPrePass = dynamic_cast<ShadowPrePass*>(m_pPasses.back().get());
		ASSERT(m_pShadowPrePass);
	}
	// *3* Z-PrePass
	{
		std::unique_ptr<Z_PrePass> _Z_PrePass = std::make_unique<Z_PrePass>("Z_PrePass", &m_RootSignature);
		appendPass(std::move(_Z_PrePass));
		// m_pZ_PrePass = (Z_PrePass*)m_pPasses.back().get();
		m_pZ_PrePass = dynamic_cast<Z_PrePass*>(m_pPasses.back().get());
		ASSERT(m_pZ_PrePass);
	}
	// *4* SSAOPass
	{
		std::unique_ptr<SSAOPass> _SSAOPass = std::make_unique<SSAOPass>("SSAOPass");
		appendPass(std::move(_SSAOPass));
		m_pSSAOPass = dynamic_cast<SSAOPass*>(m_pPasses.back().get());
		ASSERT(m_pSSAOPass);
	}
	// *5* FillLightGridPass
	{
		std::unique_ptr<FillLightGridPass> _FillLightGridPass = std::make_unique<FillLightGridPass>("FillLightGridPass");
		appendPass(std::move(_FillLightGridPass));
		m_pFillLightGridPass = dynamic_cast<FillLightGridPass*>(m_pPasses.back().get());
		ASSERT(m_pFillLightGridPass);
	}
	// *6* ShadowMappingPass
	{
		std::unique_ptr<ShadowMappingPass> _ShadowMappingPass = std::make_unique<ShadowMappingPass>("ShadowMappingPass", &m_RootSignature);
		appendPass(std::move(_ShadowMappingPass));
		m_pShadowMappingPass = dynamic_cast<ShadowMappingPass*>(m_pPasses.back().get());
		ASSERT(m_pShadowMappingPass);
	}
	// *7* MainRenderPass
	{
		std::unique_ptr<MainRenderPass> _MainRenderPass = std::make_unique<MainRenderPass>("MainRenderPass");
		appendPass(std::move(_MainRenderPass));
		m_pMainRenderPass = dynamic_cast<MainRenderPass*>(m_pPasses.back().get());
		ASSERT(m_pMainRenderPass);
	}

	finalize();

	m_pCurrentActiveCamera = nullptr;
	m_pMainLights = nullptr;
}

MasterRenderGraph::~MasterRenderGraph()
{
}

void MasterRenderGraph::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	ASSERT(m_pCurrentActiveCamera);

	BaseContext.SetMainCamera(*m_pCurrentActiveCamera);

	for (auto& p : m_pPasses)
	{
		if (p->m_bActive)
		{
			p->Execute(BaseContext);
		}
	}
}

void MasterRenderGraph::ShowPassesWindows()
{
	for (auto& _Pass : m_pPasses)
	{
		_Pass->RenderWindow();
	}
}

void MasterRenderGraph::Reset() noexcept
{
	for (auto& p : m_pPasses)
	{
		p->Reset();
	}
}

void MasterRenderGraph::Profiling()
{
	for (auto& p : m_pPasses)
	{
		printf("%s's operational time : %.5f\n", p->GetRegisteredName().c_str(), p->m_DeltaTime);
	}
	printf("\n");
}

// 현재 할거, 기억나는 거
// 할거1. D3D12Engine의 VSConstantsUpdate(), PSConstantsUpdate()를, MasterRenderGraph::Update()로 통합.
// 기억나는거1. CommandContext의 멤버변수 Camera* m_pMainCamera,     ShadowCamera* m_pMainLightShadowCamera
//                                      VSConstants m_VSConstants, PSConstants m_PSConstants 는 그대로 유지하기로 했었나?
// 기억나는거2. MasterRenderGraph에 멤버변수를 추가를 해야하나? 
//             아니면 Update()에 파라미터를 같이 받아서 그걸로 그냥 업데이를 해야하나? <- 이게 괜찮은 듯
// 기억나는거3. 만약에 CommandContext의 멤버변수들 4개를 다 빼버리면?
//             각각 Pass들이 Camera, Constants들을 각각 가져야되는데 그러면 너무 비효율적임.
//             현재가 더 나은듯. 어차피 각 패스들이 모두 똑같은거를 써야되잖아.
// 기억나는거4. 여기 Update()에서 BaseContext가 활성화된 카메라를 담으면,
//             나중에 그림자 매핑할 때, 활성화된 카메라를 광원 시점에서 그려야되는데, 
//             그러면 활성화된 카메라(메인카메라)를 잠시 ShadowMappingCamera::Execute() 스택 안에 잠시 담아놔야되는 문제가 생김.
// 기억나는거5. SetmainCamera()로 VSConstants의 modelToProjection, viewerPos를 채우는데
//                                             modelToShadow는 메인라이트잖아? 그거는 언제 채워? 채우는 시점은? 지금 Update()할 때 같이 채워져야되지 않나.

void MasterRenderGraph::Update()
{
	ASSERT(m_pCurrentActiveCamera != nullptr);
	ASSERT(m_pMainLights != nullptr);

	custom::CommandContext& BaseContext = custom::CommandContext::Begin(L"MasterRenderGraph::Update()");

	{
		MainLight FirstLight = m_pMainLights->front();
		m_pShadowMappingPass->SetLightDirection(FirstLight.GetMainLightDirection());
		BaseContext.SetPSConstants(FirstLight); // PS
	}

	BaseContext.SetMainCamera(*m_pCurrentActiveCamera); // VS
	BaseContext.SetShadowTexelSize(1 / bufferManager::g_ShadowBuffer.GetWidth()); // PS
	BaseContext.SetSpecificLightIndex(m_pLightPrePass->GetFirstConeLightIndex(), m_pLightPrePass->GetFirstConeShadowedLightIndex()); // PS
	BaseContext.SetTileDimension(bufferManager::g_SceneColorBuffer.GetWidth(), bufferManager::g_SceneColorBuffer.GetHeight(), m_pFillLightGridPass->m_WorkGroupSize);
	BaseContext.SetSync();
	BaseContext.Finish();
}

void MasterRenderGraph::appendPass(std::unique_ptr<Pass> pass)
{
	for (const auto& p : m_pPasses)
	{
		if (pass->GetRegisteredName() == p->GetRegisteredName())
		{
			ASSERT(false, "Pass name already exists : ", pass->GetRegisteredName())
		}
	}

	m_pPasses.push_back(std::move(pass));
}

RenderQueuePass& MasterRenderGraph::FindRenderQueuePass(const std::string& RenderQueuePassName)
{
	for (const auto& p : m_pPasses)
	{
		if (p->GetRegisteredName() == RenderQueuePassName)
		{
			return dynamic_cast<RenderQueuePass&>(*p);
		}
	}

	ASSERT(false, "Cannot Find Given Pass Name.");

	RenderQueuePass emptyPass{"Empty"};

	return emptyPass;
}

Pass& MasterRenderGraph::FindPass(const std::string& PassName)
{
	const auto i = std::find_if(m_pPasses.begin(), m_pPasses.end(),
		[&PassName](auto& p) { return p->GetRegisteredName() == PassName; });

	ASSERT(i == m_pPasses.end(), "Failed to find pass name");

	return **i;
}

void MasterRenderGraph::finalize()
{
	for (const auto& p : m_pPasses)
	{
		p->finalize();
	}
}

void MasterRenderGraph::BindMainCamera(Camera* pCamera)
{
	ASSERT(pCamera != nullptr);
	m_pCurrentActiveCamera = pCamera;
}

void MasterRenderGraph::BindMainLightContainer(std::vector<MainLight>* MainLightContainer)
{
	ASSERT(MainLightContainer != nullptr);
	m_pMainLights = MainLightContainer;
}