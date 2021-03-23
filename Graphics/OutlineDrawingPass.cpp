#include "stdafx.h"
#include "Device.h"
#include "CommandQueue.h"
#include "CommandQueueManager.h"
#include "OutlineDrawingPass.h"
#include "CommandContext.h"
#include "ComputeContext.h"
#include "RenderQueuePass.h"
#include "BufferManager.h"
#include "PremadePSO.h"
#include "CustomImgui.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GaussianBlurCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Paper2VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Paper2PS.h"

#include "../x64/Debug/Graphics(.lib)/CompiledShaders/OutlineWithBlurPS.h"

#elif !defined(_DEBUG) | defined(RELEASE)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_PS.h"

#include "../x64/Release/Graphics(.lib)/CompiledShaders/GaussianBlurCS.h"

#include "../x64/Release/Graphics(.lib)/CompiledShaders/Paper2VS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Paper2PS.h"

#include "../x64/Release/Graphics(.lib)/CompiledShaders/OutlineWithBlurPS.h"
#endif

#ifndef OutlineMaskStepIndex
#define OutlineMaskStepIndex 0u
#endif
#ifndef OutlineDrawStepIndex
#define OutlineDrawStepIndex 1u
#endif

OutlineDrawingPass* OutlineDrawingPass::s_pOutlineDrawingPass = nullptr;

OutlineDrawingPass::OutlineDrawingPass
(
	std::string Name,
	custom::RootSignature* pRootSignature,
	GraphicsPSO* pOutlineMaskPSO,
	GraphicsPSO* pOutlineDrawPSO
) :
	ID3D12RenderQueuePass(Name, JobFactorization::ByStep),
	m_pRootSignature(pRootSignature),
	m_pOutlineMaskPSO(pOutlineMaskPSO),
	m_pOutlineDrawPSO(pOutlineDrawPSO)
{
	ASSERT(s_pOutlineDrawingPass == nullptr);
	s_pOutlineDrawingPass = this;

	DXGI_FORMAT ColorFormat   = bufferManager::g_SceneColorBuffer.GetFormat();
	DXGI_FORMAT StencilFormat = bufferManager::g_OutlineBuffer.GetFormat();
	DXGI_FORMAT DepthFormat   = bufferManager::g_SceneDepthBuffer.GetFormat();

	if (m_pRootSignature == nullptr)
	{
		m_pRootSignature = new custom::RootSignature();

		m_pRootSignature->Reset(6, 2);
		m_pRootSignature->InitStaticSampler(0, premade::g_DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		m_pRootSignature->InitStaticSampler(1, premade::g_SamplerShadowDesc,  D3D12_SHADER_VISIBILITY_PIXEL);
		(*m_pRootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b0)
		(*m_pRootSignature)[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX); // vsConstants(b1)
		(*m_pRootSignature)[2].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b0)
		(*m_pRootSignature)[3].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);  // psConstants(b1)
		(*m_pRootSignature)[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
		(*m_pRootSignature)[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL);
		m_pRootSignature->Finalize(L"OutlineDrawingPass_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_bAllocateRootSignature = true;
	}
	if (pOutlineMaskPSO == nullptr)
	{
		m_pOutlineMaskPSO = new GraphicsPSO();

		m_pOutlineMaskPSO->SetRootSignature        (*m_pRootSignature);
		m_pOutlineMaskPSO->SetRasterizerState      (premade::g_RasterizerBackSided);
		m_pOutlineMaskPSO->SetDepthStencilState    (premade::g_DepthStencilWrite);
		m_pOutlineMaskPSO->SetRenderTargetFormats  (0, nullptr, DepthFormat);
		m_pOutlineMaskPSO->SetBlendState           (premade::g_BlendDisable);
		m_pOutlineMaskPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pOutlineMaskPSO->SetInputLayout          (5u, premade::g_InputElements);
		m_pOutlineMaskPSO->SetVertexShader         (g_pFlat_VS, sizeof(g_pFlat_VS));
		m_pOutlineMaskPSO->Finalize(L"InitAtOutlinePass_Mask");

		m_bAllocateOutlineMaskPSO = true;
	}
	if (pOutlineDrawPSO == nullptr)
	{
		m_pOutlineDrawPSO = new GraphicsPSO();
		m_pOutlineDrawPSO->SetRootSignature        (*m_pRootSignature);
		m_pOutlineDrawPSO->SetRasterizerState      (premade::g_RasterizerBackSided);
		m_pOutlineDrawPSO->SetDepthStencilState    (premade::g_DepthStencilMask);
		m_pOutlineDrawPSO->SetRenderTargetFormat   (StencilFormat, DXGI_FORMAT_UNKNOWN);
		m_pOutlineDrawPSO->SetBlendState           (premade::g_BlendOutlineDrawing);
		m_pOutlineDrawPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_pOutlineDrawPSO->SetInputLayout          (5u, premade::g_InputElements);
		m_pOutlineDrawPSO->SetVertexShader         (g_pFlat_VS, sizeof(g_pFlat_VS));
		m_pOutlineDrawPSO->SetPixelShader          (g_pFlat_PS, sizeof(g_pFlat_PS));
		m_pOutlineDrawPSO->Finalize(L"InitAtOutlinePass_Draw");

		m_bAllocateOutlineDrawPSO = true;
	}
	// BlurOutlineBuffer CS - RS
	m_BlurRootSignature.Reset(3, 2);
	m_BlurRootSignature.InitStaticSampler(0, premade::g_SamplerLinearMirrorDesc, D3D12_SHADER_VISIBILITY_PIXEL); // Vertical
	m_BlurRootSignature.InitStaticSampler(1, premade::g_SamplerPointMirrorDesc, D3D12_SHADER_VISIBILITY_PIXEL);  // Horizontal
	m_BlurRootSignature[0].InitAsConstantBuffer(0);
	m_BlurRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
	m_BlurRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_BlurRootSignature.Finalize(L"BlurRootSignature");

	// BlurOutlineBuffer CS-PSO
	m_GaussBlurPSO.SetRootSignature(m_BlurRootSignature);
	m_GaussBlurPSO.SetComputeShader(g_pGaussianBlurCS, sizeof(g_pGaussianBlurCS));
	m_GaussBlurPSO.Finalize(L"OutlineDrawingPass-GaussBlurCSPSO");

	m_PaperOutlinePSO.SetRootSignature        (m_BlurRootSignature);
	m_PaperOutlinePSO.SetRasterizerState      (premade::g_RasterizerBackSided);
	m_PaperOutlinePSO.SetDepthStencilState    (premade::g_DepthStencilMask);
	m_PaperOutlinePSO.SetRenderTargetFormat   (ColorFormat, DepthFormat);
	m_PaperOutlinePSO.SetBlendState           (premade::g_BlendOutlineDrawing);
	m_PaperOutlinePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PaperOutlinePSO.SetInputLayout          (0, nullptr);
	m_PaperOutlinePSO.SetVertexShader         (g_pPaper2VS, sizeof(g_pPaper2VS));
	m_PaperOutlinePSO.SetPixelShader          (g_pPaper2PS, sizeof(g_pPaper2PS));
	m_PaperOutlinePSO.Finalize(L"PaperOutlinePSO");
	
	m_HorizontalOutlineBlurPSO.SetRootSignature(m_BlurRootSignature);
	m_HorizontalOutlineBlurPSO.SetRasterizerState(premade::g_RasterizerBackSided);
	// m_HorizontalOutlineBlurPSO.SetDepthStencilState(premade::g_DepthStencilMask);
	m_HorizontalOutlineBlurPSO.SetDepthStencilState(premade::g_DepthStateDisabled);
	m_HorizontalOutlineBlurPSO.SetRenderTargetFormat(ColorFormat, DXGI_FORMAT_UNKNOWN);
	m_HorizontalOutlineBlurPSO.SetBlendState(premade::g_BlendDisable);
	m_HorizontalOutlineBlurPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_HorizontalOutlineBlurPSO.SetInputLayout(0, nullptr);
	m_HorizontalOutlineBlurPSO.SetVertexShader(g_pPaper2VS, sizeof(g_pPaper2VS));
	m_HorizontalOutlineBlurPSO.SetPixelShader(g_pOutlineWithBlurPS, sizeof(g_pOutlineWithBlurPS));
	m_HorizontalOutlineBlurPSO.Finalize(L"HorizontalOutlineBlurPSO");

	m_VerticalOutlineBlurPSO.SetRootSignature(m_BlurRootSignature); 
	m_VerticalOutlineBlurPSO.SetRasterizerState(premade::g_RasterizerBackSided);
	m_VerticalOutlineBlurPSO.SetDepthStencilState(premade::g_DepthStencilMask);
	m_VerticalOutlineBlurPSO.SetRenderTargetFormat(ColorFormat, DepthFormat);
	m_VerticalOutlineBlurPSO.SetBlendState(premade::g_BlendOutlineDrawing);
	m_VerticalOutlineBlurPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_VerticalOutlineBlurPSO.SetInputLayout(0, nullptr);
	m_VerticalOutlineBlurPSO.SetVertexShader(g_pPaper2VS, sizeof(g_pPaper2VS));
	m_VerticalOutlineBlurPSO.SetPixelShader(g_pOutlineWithBlurPS, sizeof(g_pOutlineWithBlurPS));
	m_VerticalOutlineBlurPSO.Finalize(L"VerticalOutlineBlurPSO");
	
	m_BlurConstants.BlurSigma     = 2.0f;
	m_BlurConstants.BlurRadius    = 5;
	m_BlurConstants.BlurCount     = 5u;
	m_BlurConstants.BlurIntensity = 0.23f;

	m_BlurConstants2.BlurSigma     = 2.0f;
	m_BlurConstants2.BlurRadius    = 5;
	m_BlurConstants2.BlurDirection = 0; // If 0, is Vertical, else Horizontal.
	m_BlurConstants2.BlurIntensity = 0.0f;
}

OutlineDrawingPass::~OutlineDrawingPass()
{
	if (m_bAllocateRootSignature && (m_pRootSignature != nullptr))
	{
		delete m_pRootSignature;
		m_pRootSignature = nullptr;
	}

	if (m_bAllocateOutlineMaskPSO && (m_pOutlineMaskPSO != nullptr))
	{
		delete m_pOutlineMaskPSO;
		m_pOutlineMaskPSO = nullptr;
	}

	if (m_bAllocateOutlineDrawPSO && (m_pOutlineDrawPSO != nullptr))
	{
		delete m_pOutlineDrawPSO;
		m_pOutlineDrawPSO = nullptr;
	}
}

void OutlineDrawingPass::ExecutePass() DEBUG_EXCEPT
{
	if (!GetJobCount())
	{
		return;
	}

	// m_bUseComputeShaderVersion 키고 끄고할 때의 확인할 수 있는거
	// CommandList, CommandAllocator 줄이고 늘릴 때 Begin()부터 디버깅 가능.
	custom::GraphicsContext& graphicsContext = custom::GraphicsContext::Begin(5 - m_bUseComputeShaderVersion);
	uint8_t MaxCommandIndex = graphicsContext.GetNumCommandLists() - 1;
	graphicsContext.SetResourceTransitionBarrierIndex(0u);

	custom::ThreadPool::EnqueueVariadic(SetParamsWithVariadic, 4, this, &graphicsContext, 1, MaxCommandIndex);

	custom::CommandQueue& ComputeQueue = device::g_commandQueueManager.GetComputeQueue();
	custom::CommandQueue& GraphicsQueue = device::g_commandQueueManager.GetGraphicsQueue();

	graphicsContext.PIXBeginEvent(L"OutlineDrawingPass", 0);

	graphicsContext.SetResourceTransitionBarrierIndex(1u);
	ExecuteStencilMasking(graphicsContext, bufferManager::g_SceneDepthBuffer, 1); // Stage 1
	graphicsContext.SetResourceTransitionBarrierIndex(2u);
	ExecuteDrawColor     (graphicsContext, bufferManager::g_OutlineBuffer, 2);    // Stage 2
	graphicsContext.ExecuteCommands(2u, true, true);

	if (m_bUseComputeShaderVersion)
	{
		// Stage 3, 4
		custom::ComputeContext computeContext = custom::ComputeContext::Begin(1);
		computeContext.SetResourceTransitionBarrierIndex(0u);
		ExecuteBlurOutlineBuffer(computeContext, bufferManager::g_OutlineBuffer, bufferManager::g_OutlineHelpBuffer, 0);

		ASSERT(ComputeQueue.WaitCommandQueueCompletedGPUSide(GraphicsQueue));
		computeContext.Finish(false);
		
		ASSERT(GraphicsQueue.WaitCommandQueueCompletedGPUSide(ComputeQueue)); // TODO 1 : 여기 Wait 지워도 볼것.
		
		ExecutePaperOutline(graphicsContext, bufferManager::g_SceneColorBuffer, bufferManager::g_SceneDepthBuffer, bufferManager::g_OutlineBuffer, 3);
		graphicsContext.PIXEndEvent(MaxCommandIndex); // End OutlineDrawingPass
		graphicsContext.Finish(false);
	}
	else
	{
		// Stage 3, 4
		graphicsContext.SetResourceTransitionBarrierIndex(3u);
		ExecuteHorizontalBlur      (graphicsContext, bufferManager::g_OutlineBuffer,    bufferManager::g_OutlineHelpBuffer, 3);
		graphicsContext.SetResourceTransitionBarrierIndex(4u);
		ExecuteVerticalBlurAndPaper(graphicsContext, bufferManager::g_SceneColorBuffer, bufferManager::g_SceneDepthBuffer, bufferManager::g_OutlineHelpBuffer, 4);
		graphicsContext.SetResourceTransitionBarrierIndex(0u);
		graphicsContext.PIXEndEvent(MaxCommandIndex); // End OutlineDrawingPass
		graphicsContext.Finish(false);
	}
}

void OutlineDrawingPass::RenderWindow()
{
	ImGui::BeginChild("OutlineDrawing Pass");

	static const char* BlurTypeTag[2] = { "UsingComputeShader", "UsingPixelShader" };
	static const char* CurrentBlurType = BlurTypeTag[0];

	bool bTypeChanged = false;

	if (ImGui::BeginCombo("Blur Type", CurrentBlurType))
	{
		for (size_t i = 0; i < _countof(BlurTypeTag); ++i)
		{
			const bool bSelected = (CurrentBlurType == BlurTypeTag[i]);
			if (ImGui::Selectable(BlurTypeTag[i], bSelected))
			{
				bTypeChanged = true;
				CurrentBlurType = BlurTypeTag[i];

				if (CurrentBlurType == BlurTypeTag[0])
				{
					m_bUseComputeShaderVersion = true;
				}
				else if (CurrentBlurType == BlurTypeTag[1])
				{
					m_bUseComputeShaderVersion = false;
				}
			}
			if (bTypeChanged)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::CenteredSeparator();

	if (m_bUseComputeShaderVersion)
	{
		ImGui::SliderFloat("Sigma", &m_BlurConstants.BlurSigma, 0.1f, 5.0f);
		ImGui::SliderInt("Radius", &m_BlurConstants.BlurRadius, 1, 5);
		ImGui::SliderInt("Iteration", &m_BlurConstants.BlurCount, 1, 5);
		ImGui::SliderFloat("Intensity", &m_BlurConstants.BlurIntensity, 0.01f, 1.0f);
	}
	else
	{
		ImGui::SliderFloat("Sigma", &m_BlurConstants2.BlurSigma, 0.1f, 5.0f);
		ImGui::SliderInt("Radius", &m_BlurConstants2.BlurRadius, 1, 5);
		ImGui::SliderFloat("Intensity", &m_BlurConstants2.BlurIntensity, 0.01f, 1.0f);
	}

	ImGui::EndChild();
}

// Stage 1
void OutlineDrawingPass::ExecuteStencilMasking(custom::GraphicsContext& graphicsContext, DepthBuffer& TargetBuffer, uint8_t commandIndex)
{
	// Stencil Mask 
	// Target : g_SceneDepthBuffer-Stencil

	graphicsContext.PIXBeginEvent(L"Outline Masking", commandIndex);

	graphicsContext.SetViewportAndScissor(TargetBuffer, commandIndex);
	graphicsContext.SetRootSignature(*m_pRootSignature, commandIndex);
	graphicsContext.SetPipelineState(*m_pOutlineMaskPSO, commandIndex);
	graphicsContext.TransitionResource(TargetBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_BARRIER_STENCIL_SUBRESOURCE);
	graphicsContext.SubmitResourceBarriers(commandIndex);
	graphicsContext.ClearDepth(TargetBuffer, commandIndex);
	graphicsContext.SetOnlyDepthStencil(TargetBuffer.GetDSV(), commandIndex);
	graphicsContext.SetStencilRef(0xff, commandIndex);

	ID3D12RenderQueuePass::ExecuteWithRange(graphicsContext, commandIndex, OutlineMaskStepIndex, OutlineMaskStepIndex);

	graphicsContext.PIXEndEvent(commandIndex); // End Outline Masking
}

// Stage 2.
void OutlineDrawingPass::ExecuteDrawColor(custom::GraphicsContext& graphicsContext, ColorBuffer& TargetBuffer, uint8_t commandIndex)
{
	// Draw
	// Target : g_OutlineBuffer
	graphicsContext.PIXBeginEvent(L"Outline Drawing", commandIndex);
	graphicsContext.SetViewportAndScissor(TargetBuffer, commandIndex);
	graphicsContext.SetRootSignature(*m_pRootSignature, commandIndex);
	graphicsContext.SetPipelineState(*m_pOutlineDrawPSO, commandIndex);
	graphicsContext.TransitionResource(TargetBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, 0U);
	graphicsContext.SubmitResourceBarriers(commandIndex);
	graphicsContext.ClearColor(TargetBuffer, commandIndex);
	graphicsContext.SetRenderTarget(TargetBuffer.GetRTV(), commandIndex);
	ID3D12RenderQueuePass::ExecuteWithRange(graphicsContext, commandIndex, OutlineDrawStepIndex, OutlineDrawStepIndex);

	graphicsContext.PIXEndEvent(commandIndex); // End Outline Drawing
}

// Stage 3. (ComputeContext)
void OutlineDrawingPass::ExecuteBlurOutlineBuffer
(
	custom::ComputeContext& computeContext, 
	ColorBuffer& TargetBuffer, 
	ColorBuffer& HelpBuffer, 
	uint8_t commandIndex
)
{
	uint32_t TargetBufferHeight = TargetBuffer.GetHeight();
	uint32_t TargetBufferWidth  = TargetBuffer.GetWidth();

	uint32_t HelpBufferHeight = HelpBuffer.GetHeight();
	uint32_t HelpBufferWidth  = HelpBuffer.GetWidth();

	ASSERT
	(
		(TargetBufferHeight == HelpBufferHeight) && 
		(TargetBufferWidth  == HelpBufferWidth)
	);

	computeContext.PIXBeginEvent(L"GaussianBlur", commandIndex);
	computeContext.SetRootSignature(m_BlurRootSignature, commandIndex);
	computeContext.SetPipelineState(m_GaussBlurPSO, commandIndex);
	computeContext.TransitionResource(TargetBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
	computeContext.TransitionResource(HelpBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
	computeContext.SubmitResourceBarriers(commandIndex);
	computeContext.SetDynamicConstantBufferView(0, sizeof(m_BlurConstants), &m_BlurConstants, commandIndex);
	
	D3D12_CPU_DESCRIPTOR_HANDLE UAVs[2] =
	{
		TargetBuffer.GetUAV(),
		HelpBuffer.GetUAV()
	};

	computeContext.SetDynamicDescriptors(1, 0, _countof(UAVs), UAVs, commandIndex);

	UINT numThreadsGroupX;
	UINT numThreadsGroupY;

	if (TargetBufferHeight < TargetBufferWidth)
	{
		numThreadsGroupX = (UINT)ceilf(TargetBufferWidth / 256.0f);
		numThreadsGroupY = TargetBufferWidth;
	}
	else
	{

		numThreadsGroupX = (UINT)ceilf(TargetBufferHeight / 256.0f);
		numThreadsGroupY = TargetBufferHeight;
	}

	computeContext.ClearUAV(HelpBuffer, commandIndex);
	computeContext.Dispatch2D(numThreadsGroupX, numThreadsGroupY, 1U, 1U, commandIndex);
	computeContext.PIXEndEvent(commandIndex); // End GaussianBlur
}

// Step4 (After ComputeShader Ver.)
void OutlineDrawingPass::ExecutePaperOutline
(
	custom::GraphicsContext& graphicsContext,
	ColorBuffer& TargetBuffer,
	DepthBuffer& DepthStencilBuffer,
	ColorBuffer& SrcBuffer,
	uint8_t commandIndex
)
{
	uint32_t TargetBufferHeight = TargetBuffer.GetHeight();
	uint32_t TargetBufferWidth = TargetBuffer.GetWidth();

	uint32_t HelpBufferHeight = SrcBuffer.GetHeight();
	uint32_t HelpBufferWidth = SrcBuffer.GetWidth();

	ASSERT
	(
		(TargetBufferHeight - (HelpBufferHeight << 1)) <= 1 &&
		(TargetBufferWidth - (HelpBufferWidth << 1)) <= 1
	);

	graphicsContext.PIXBeginEvent(L"PaperOutline", commandIndex);

	graphicsContext.SetRootSignature(m_BlurRootSignature, commandIndex);
	graphicsContext.SetPipelineState(m_PaperOutlinePSO, commandIndex);
	graphicsContext.SetViewportAndScissor(TargetBuffer, commandIndex);
	graphicsContext.TransitionResource(TargetBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, 0U);
	graphicsContext.TransitionResource(DepthStencilBuffer, D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_BARRIER_STENCIL_SUBRESOURCE);
	graphicsContext.TransitionResource(SrcBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0U);
	graphicsContext.SubmitResourceBarriers(commandIndex);
	graphicsContext.SetRenderTarget(TargetBuffer.GetRTV(), DepthStencilBuffer.GetDSV_StencilReadOnly(), commandIndex);
	graphicsContext.SetStencilRef(0xff, commandIndex);

	D3D12_CPU_DESCRIPTOR_HANDLE UAVs[2] =
	{
		TargetBuffer.GetUAV(),
		SrcBuffer.GetUAV()
	};
	graphicsContext.SetDynamicDescriptors(1, 0, _countof(UAVs), UAVs, commandIndex);
	graphicsContext.SetDynamicDescriptor(2, 0, SrcBuffer.GetSRV(), commandIndex);
	graphicsContext.Draw(6, commandIndex);

	graphicsContext.PIXEndEvent(commandIndex); // End PaperOutline;
}

// Stage 3. (GraphicsContext)
void OutlineDrawingPass::ExecuteHorizontalBlur(custom::GraphicsContext& graphicsContext, ColorBuffer& OutlineBuffer, ColorBuffer& HelpBuffer, uint8_t commandIndex)
{
	graphicsContext.PIXBeginEvent(L"Horizontal Blur", commandIndex);
	graphicsContext.SetRootSignature(m_BlurRootSignature, commandIndex);
	graphicsContext.SetPipelineState(m_HorizontalOutlineBlurPSO, commandIndex);
	graphicsContext.TransitionResource(OutlineBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0U);
	graphicsContext.TransitionResource(HelpBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, 0U);
	graphicsContext.SubmitResourceBarriers(commandIndex);
	graphicsContext.ClearColor(HelpBuffer, commandIndex);
	graphicsContext.SetRenderTarget(HelpBuffer.GetRTV(), commandIndex);
	graphicsContext.SetViewportAndScissor(HelpBuffer, commandIndex);

	{
		m_BlurConstants2.BlurDirection = 1;
		graphicsContext.SetDynamicConstantBufferView(0u, sizeof(m_BlurConstants2), &m_BlurConstants2, commandIndex);
		graphicsContext.SetDynamicDescriptor(2, 0, OutlineBuffer.GetSRV(), commandIndex);

		D3D12_CPU_DESCRIPTOR_HANDLE UAVs[2] =
		{
			OutlineBuffer.GetUAV(),
			HelpBuffer.GetUAV()
		};

		graphicsContext.SetDynamicDescriptors(1, 0, _countof(UAVs), UAVs, commandIndex);
	}

	graphicsContext.Draw(6u, commandIndex);
	graphicsContext.PIXEndEvent(commandIndex); // End Horizontal Blur
}

// Step4 (GraphicsContext Ver.)
void OutlineDrawingPass::ExecuteVerticalBlurAndPaper(custom::GraphicsContext& graphicsContext, ColorBuffer& TargetBuffer, DepthBuffer& DepthStencilBuffer, ColorBuffer& HelpBuffer, uint8_t commandIndex)
{
	graphicsContext.PIXBeginEvent(L"VerticalBlur And Paper", commandIndex);
	graphicsContext.SetViewportAndScissor(TargetBuffer, commandIndex);
	graphicsContext.SetRootSignature(m_BlurRootSignature, commandIndex);
	graphicsContext.SetPipelineState(m_VerticalOutlineBlurPSO, commandIndex);
	graphicsContext.TransitionResource(DepthStencilBuffer, D3D12_RESOURCE_STATE_DEPTH_READ, 1U);
	graphicsContext.TransitionResource(HelpBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0U);
	graphicsContext.TransitionResource(TargetBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, 0U);
	graphicsContext.SubmitResourceBarriers(commandIndex);
	graphicsContext.SetRenderTarget(TargetBuffer.GetRTV(), DepthStencilBuffer.GetDSV_StencilReadOnly(), commandIndex);
	{
		m_BlurConstants2.BlurDirection = 0;
		graphicsContext.SetDynamicConstantBufferView(0u, sizeof(m_BlurConstants2), &m_BlurConstants2, commandIndex);
		graphicsContext.SetDynamicDescriptor(2, 0, HelpBuffer.GetSRV(), commandIndex);

		D3D12_CPU_DESCRIPTOR_HANDLE UAVs[2] =
		{
			TargetBuffer.GetUAV(),
			HelpBuffer.GetUAV()
		};

		graphicsContext.SetDynamicDescriptors(1, 0, _countof(UAVs), UAVs, commandIndex);
	}
	graphicsContext.Draw(6u, commandIndex);
	graphicsContext.PIXEndEvent(commandIndex); // End Horizontal Blur
}


