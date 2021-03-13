#include "stdafx.h"
#include "MasterRenderGraph.h"

#include "CommandContextManager.h"
#include "ShadowBuffer.h"
#include "BufferManager.h"
#include "PreMadePSO.h"
#include "GPUWorkManager.h"
#include "ObjectFilterFlag.h"
#include "ShaderConstantsTypeDefinitions.h"

#include "Camera.h"
#include "ShadowCamera.h"

#include "RenderQueuePass.h"
#include "LightPrePass.h"
#include "ShadowPrePass.h"
#include "Z_PrePass.h"
#include "SSAOPass.h"
#include "FillLightGridPass.h"
#include "ShadowMappingPass.h"
#include "MainRenderPass.h"
#include "OutlineDrawingPass.h"
#include "DebugWireFramePass.h"
#include "DebugShadowMapPass.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthVS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DepthPS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_Basic.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_TexCoord.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelVS_TexNormal.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_Basic.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_TexCoord.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ModelPS_TexNormal.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"
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
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/OutlineWithBlurPS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/HoriontalBlurCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/VerticalBlurCS.h"
#endif

MasterRenderGraph* MasterRenderGraph::s_pMasterRenderGraph = nullptr;

MasterRenderGraph::MasterRenderGraph()
	:
	m_SelctedPassIndex(0ul),
	m_pMainLights         (nullptr),
	m_pCurrentActiveCamera(nullptr),
	m_bFullScreenDebugPasses(false),
	m_bDebugPassesMode      (false),
	m_bDebugShadowMap       (false),
	m_bSSAOPassEnable       (true),
	m_bSSAODebugDraw        (false),
	m_bShadowMappingPass    (true),
	m_bMainRenderPass       (true),
	m_bGPUTaskFiberDirty      (true),
	m_NumNeedCommandQueues  (1U),
	m_NumNeedCommandALs     (1U),
	m_pLightPrePass      (nullptr),
	m_pShadowPrePass     (nullptr),
	m_pZ_PrePass         (nullptr),
	m_pSSAOPass          (nullptr),
	m_pFillLightGridPass (nullptr),
	m_pShadowMappingPass (nullptr),
	m_pMainRenderPass    (nullptr),
	m_pOutlineDrawingPass(nullptr),
	m_pDebugWireFramePass(nullptr),
	m_pDebugShadowMapPass(nullptr)
{
	ASSERT(s_pMasterRenderGraph == nullptr);
	s_pMasterRenderGraph = this;

	// TODO 1 : 
	// 1. RootSignature 통합 (최대한 많이 쓰고, 한꺼번에 바인딩 -> 하지만 내 컴퓨터에서 Resource Tier가 낮아서 하나도 빠짐없이 바인딩해야된다)
	// 2. PSO 갯수 줄일 수 있으면 줄이기
	{
		m_RootSignature;
		m_RootSignature.Reset(6, 2); // (4, 2) -> (5, 2) -> (6, 2)
		m_RootSignature.InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);              //
		m_RootSignature.InitStaticSampler(1, premade::g_SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);               //
		m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)                   //
		m_RootSignature[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b1)                   //
		m_RootSignature[2].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)                   //
		m_RootSignature[3].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b1)                   //
		m_RootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0,  6, D3D12_SHADER_VISIBILITY_PIXEL);  // Diffuse, Specular, Emissvie, Normal, LightMap, Reflection
		m_RootSignature[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL); // texSSAO, texShadow, lightBuffer, lightShadowArrayTex, LightGrid, LightGridMask

		m_RootSignature.Finalize(L"MainRS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}

	DXGI_FORMAT ColorFormat = bufferManager::g_SceneColorBuffer.GetFormat();
	DXGI_FORMAT DepthFormat = bufferManager::g_SceneDepthBuffer.GetFormat();

	D3D12_INPUT_ELEMENT_DESC InputElements[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
		m_DepthPSO.Finalize(L"MasterRenderGraph-m_DepthPSO");
	}
	// Shadow Shader
	{
		m_ShadowPSO = m_DepthPSO;
		m_ShadowPSO.SetRasterizerState(premade::g_RasterizerShadow);
		m_ShadowPSO.SetRenderTargetFormats(0, nullptr, bufferManager::g_ShadowBuffer.GetFormat());
		m_ShadowPSO.Finalize(L"MasterRenderGraph-m_ShadowPSO");
	}
	// MainPass Shader
	{
		m_MainRenderPSO = m_DepthPSO;
		m_MainRenderPSO.SetBlendState(premade::g_BlendDisable);
		m_MainRenderPSO.SetDepthStencilState(premade::g_DepthStateTestEqual);
		m_MainRenderPSO.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
		m_MainRenderPSO.SetVertexShader(g_pModelVS_TexNormal, sizeof(g_pModelVS_TexNormal));
		m_MainRenderPSO.SetPixelShader(g_pModelPS_TexNormal, sizeof(g_pModelPS_TexNormal));
		m_MainRenderPSO.Finalize(L"MasterRenderGraph-m_MainRenderPSO");
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
		std::unique_ptr<ShadowPrePass> _ShadowPrePass = std::make_unique<ShadowPrePass>("ShadowPrePass");
		appendPass(std::move(_ShadowPrePass));
		// m_pShadowPrePass = (ShadowPrePass*)m_pPasses.back().get();
		m_pShadowPrePass = dynamic_cast<ShadowPrePass*>(m_pPasses.back().get());
		ASSERT(m_pShadowPrePass);
	}
	// *3* Z-PrePass
	{
		std::unique_ptr<Z_PrePass> _Z_PrePass = std::make_unique<Z_PrePass>("Z_PrePass");
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
		std::unique_ptr<ShadowMappingPass> _ShadowMappingPass = std::make_unique<ShadowMappingPass>("ShadowMappingPass");
		appendPass(std::move(_ShadowMappingPass));
		m_pShadowMappingPass = dynamic_cast<ShadowMappingPass*>(m_pPasses.back().get());
		ASSERT(m_pShadowMappingPass);
	}
	// OutlineDrawingPass
	
	// *7* MainRenderPass
	{
		std::unique_ptr<MainRenderPass> _MainRenderPass = std::make_unique<MainRenderPass>("MainRenderPass");
		appendPass(std::move(_MainRenderPass));
		m_pMainRenderPass = dynamic_cast<MainRenderPass*>(m_pPasses.back().get());
		ASSERT(m_pMainRenderPass);
	}
	{
		std::unique_ptr<OutlineDrawingPass> _OutlineDrawingPass = std::make_unique<OutlineDrawingPass>("OutlineDrawingPass");
		appendPass(std::move(_OutlineDrawingPass));
		m_pOutlineDrawingPass = dynamic_cast<OutlineDrawingPass*>(m_pPasses.back().get());
		ASSERT(m_pOutlineDrawingPass);
	}
#if defined(_DEBUG)
	{
		std::unique_ptr<DebugWireFramePass> _DebugWireFramePass = std::make_unique<DebugWireFramePass>("DebugWireFramePass");
		appendFullScreenDebugPass(std::move(_DebugWireFramePass));
		m_pDebugWireFramePass = dynamic_cast<DebugWireFramePass*>(m_pFullScreenDebugPasses.back().get());
		ASSERT(m_pDebugWireFramePass);
	}
	{
		std::unique_ptr<DebugShadowMapPass> _DebugShadowMapPass = std::make_unique<DebugShadowMapPass>("DebugShadowMapPass");
		appendDebugPass(std::move(_DebugShadowMapPass));
		m_pDebugShadowMapPass = dynamic_cast<DebugShadowMapPass*>(m_pDebugPasses.back().get());
		ASSERT(m_pDebugShadowMapPass);
	}
#endif

	finalize();
}

MasterRenderGraph::~MasterRenderGraph()
{
}

void MasterRenderGraph::Execute() DEBUG_EXCEPT
{
	ASSERT(m_pCurrentActiveCamera != nullptr);

	RenderPassesWindow();

	custom::CommandContext::BeginFrame();

	for (auto& p : (m_bDebugPassesMode) ? m_pFullScreenDebugPasses : m_pPasses)
	{
		if (p->m_bActive)
		{
			p->ExecutePass();
		}
	}

	for (auto& p : m_pDebugPasses)
	{
		if (p->m_bActive)
		{
			// p->Execute();
			p->ExecutePass();
		}
	}

	custom::CommandContext::EndFrame();
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

	for (auto& p : m_pFullScreenDebugPasses)
	{
		p->Reset();
	}
}

void MasterRenderGraph::Profiling()
{
#ifdef _DEBUG
	for (auto& p : m_pPasses)
	{
		printf("%s's operational time : %.5f, DDTime : %.4lf\n", p->GetRegisteredName().c_str(), p->m_DeltaTime, (double)(p->m_DeltaTime - p->m_DeltaTimeBefore));

		{
			ID3D12RenderQueuePass* pRQP = dynamic_cast<ID3D12RenderQueuePass*>(p.get());
			if (pRQP != nullptr)
			{
				printf("Job Count : %lld\n", pRQP->GetJobCount());
			}
		}

		p->m_DeltaTimeBefore = p->m_DeltaTime;
	}
	printf("\n");
#endif
}

void MasterRenderGraph::Update()
{
	ASSERT(m_pCurrentActiveCamera != nullptr);
	ASSERT(m_pMainLights != nullptr);

	CommandContextManager::SetMainCamera(*m_pCurrentActiveCamera);
	CommandContextManager::SetModelToShadow(m_pMainLights->front().GetShadowMatrix());

	MainLight& FirstLight = m_pMainLights->front();
	CommandContextManager::SetPSConstants(FirstLight);
	
	CommandContextManager::SetShadowTexelSize(1 / (float)bufferManager::g_ShadowBuffer.GetWidth());
	CommandContextManager::SetSpecificLightIndex(m_pLightPrePass->GetFirstConeLightIndex(), m_pLightPrePass->GetFirstConeShadowedLightIndex()); // PS
	CommandContextManager::SetTileDimension(bufferManager::g_SceneColorBuffer.GetWidth(), bufferManager::g_SceneColorBuffer.GetHeight(), m_pFillLightGridPass->m_WorkGroupSize);

	m_pSSAOPass->m_bEnable           = m_bSSAOPassEnable;
	m_pSSAOPass->m_bDebugDraw        = m_bSSAODebugDraw;
	m_pShadowMappingPass->m_bActive  = m_bShadowMappingPass;
	m_pMainRenderPass->m_bActive     = m_bMainRenderPass;

	m_pDebugShadowMapPass->m_bActive = m_bDebugShadowMap;
}

void MasterRenderGraph::appendPass(std::unique_ptr<D3D12Pass> _Pass)
{
	for (const auto& p : m_pPasses)
	{
		if (_Pass->GetRegisteredName() == p->GetRegisteredName())
		{
			ASSERT(false, "Pass m_Name already exists : ", _Pass->GetRegisteredName())
		}
	}

	m_pPasses.push_back(std::move(_Pass));
}
void MasterRenderGraph::appendFullScreenDebugPass(std::unique_ptr<D3D12Pass> _Pass)
{
	for (const auto& p : m_pFullScreenDebugPasses)
	{
		if (_Pass->GetRegisteredName() == p->GetRegisteredName())
		{
			ASSERT(false, "Pass m_Name already exists : ", _Pass->GetRegisteredName())
		}
	}

	m_pFullScreenDebugPasses.push_back(std::move(_Pass));
}
void MasterRenderGraph::appendDebugPass(std::unique_ptr<D3D12Pass> _Pass)
{
	for (const auto& p : m_pDebugPasses)
	{
		if (_Pass->GetRegisteredName() == p->GetRegisteredName())
		{
			ASSERT(false, "Pass m_Name already exists : ", _Pass->GetRegisteredName())
		}
	}

	m_pDebugPasses.push_back(std::move(_Pass));
}
ID3D12RenderQueuePass* MasterRenderGraph::FindRenderQueuePass(const std::string& RenderQueuePassName) const
{
	for (const auto& p : m_pPasses)
	{
		if (p->GetRegisteredName() == RenderQueuePassName)
		{
			return dynamic_cast<ID3D12RenderQueuePass*>(p.get());
		}
	}

	for (const auto& p : m_pFullScreenDebugPasses)
	{
		if (p->GetRegisteredName() == RenderQueuePassName)
		{
			return dynamic_cast<ID3D12RenderQueuePass*>(p.get());
		}
	}

	ASSERT(false, "Cannot Find Given Pass Name.");

	return nullptr;
}
D3D12Pass& MasterRenderGraph::FindPass(const std::string& PassName)
{
	const auto i = std::find_if(m_pPasses.begin(), m_pPasses.end(),
		[&PassName](auto& p) { return p->GetRegisteredName() == PassName; });

	ASSERT(i == m_pPasses.end(), "Failed to find pass m_Name");

	return **i;
}
void MasterRenderGraph::finalize()
{
	for (const auto& p : m_pPasses)
	{
		p->finalize();
	}
}
void MasterRenderGraph::RenderPassesWindow()
{
	ImGui::Begin("Passes");

	ImGui::Columns(2, 0, true);
	ImGui::BeginChild("Passes");

	// 콤보박스
	ImGui::Text("Select Pass");
	if (ImGui::BeginCombo("", m_pPasses[m_SelctedPassIndex]->GetRegisteredName().c_str()))
	{
		for (size_t PassIndex = 0; PassIndex < m_pPasses.size(); ++PassIndex)
		{
			const bool bSelected = (PassIndex == m_SelctedPassIndex);

			if (ImGui::Selectable(m_pPasses[PassIndex]->GetRegisteredName().c_str(), bSelected))
			{
				m_SelctedPassIndex = PassIndex;
			}
		}
		ImGui::EndCombo();
	}

	ImGui::EndChild(); // End Passes
	ImGui::NextColumn();
	m_pPasses[m_SelctedPassIndex]->RenderWindow();

	ImGui::End();
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
void MasterRenderGraph::ToggleSSAODebugMode()
{
	m_bSSAODebugDraw = !m_bSSAODebugDraw;

	if (m_bSSAODebugDraw)
	{
		disableShadowMappingPass();
		disableMainRenderPass();
	}
	else
	{
		enableShadowMappingPass();
		enableMainRenderPass();
	}
}
