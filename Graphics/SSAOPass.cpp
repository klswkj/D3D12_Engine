#include "stdafx.h"
#include "SSAOPass.h"
#include "PreMadePSO.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "IBaseCamera.h"
#include "Camera.h"
#include "CommandContext.h"
#include "ComputeContext.h"
#include "CommandQueueManager.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "PSO.h"
#include "Device.h"
#include "Graphics.h"
#include "BufferManager.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/AoPrepareDepthBuffers1CS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/AoPrepareDepthBuffers2CS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/LinearizeDepthCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DebugSSAOCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/AoCompute1CS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/AoCompute2CS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/AoBlurUpsampleBlendOutCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/AoBlurUpsamplePreMinBlendOutCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/AoBlurUpsampleCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/AoBlurUpsamplePreMinCS.h"
#elif !defined(_DEBUG) | defined(RELEASE)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/AoPrepareDepthBuffers1CS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/AoPrepareDepthBuffers2CS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/LinearizeDepthCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/DebugSSAOCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/AoCompute1CS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/AoCompute2CS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/AoBlurUpsampleBlendOutCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/AoBlurUpsamplePreMinBlendOutCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/AoBlurUpsampleCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/AoBlurUpsamplePreMinCS.h"
#endif

#define NUM_GROUP_THREAD_WIDTH 8
#define NUM_GROUP_THREAD_HEIGHT 8

// Linearize
// Screen-Space reconstruct
// Random sample Generation
// 

SSAOPass::SSAOPass(std::string pName)
	: Pass(pName)
{
	{
		m_MainRootSignature.Reset(4, 2);
		m_MainRootSignature.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
		m_MainRootSignature.InitStaticSampler(1, premade::g_SamplerLinearBorderDesc);
		m_MainRootSignature[0].InitAsConstants(0, 4);
		m_MainRootSignature[1].InitAsConstantBuffer(1);
		m_MainRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 5);
		m_MainRootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5);
		m_MainRootSignature.Finalize(L"SSAO");
	}
#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName.SetRootSignature(m_MainRootSignature); \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize()

	{
		CreatePSO(m_DepthPrepareCS_16, g_pAoPrepareDepthBuffers1CS);
		CreatePSO(m_DepthPrepareCS_64, g_pAoPrepareDepthBuffers2CS);
		CreatePSO(m_RenderWithWideSamplingCS, g_pAoCompute1CS);
		CreatePSO(m_RenderWithDepthArrayCS, g_pAoCompute2CS);
	}
	{
		CreatePSO(m_BlurUpSamleBlendWithHighResolutionCS, g_pAoBlurUpsampleBlendOutCS);
		CreatePSO(m_BlurUpSampleBlendWithBothCS, g_pAoBlurUpsamplePreMinBlendOutCS);
		CreatePSO(m_BlurUpSampleFinalWithNoneCS, g_pAoBlurUpsampleCS);
		CreatePSO(m_BlurUpSampleFinalWithCombineLowerResolutionCS, g_pAoBlurUpsamplePreMinCS);
	}
	{
		CreatePSO(m_LinearizeDepthCS, g_pLinearizeDepthCS);
		CreatePSO(m_DebugSSAOCS, g_pDebugSSAOCS);
	}

	m_bEnable = true;
	m_bDebugDraw = false;
	m_bAsyncCompute = false;
	m_bComputeLinearZ = true;

	m_NoiseFilterTolerance = -3.0f; // -8.0f ~ 0.0f, 0.25f
	m_BlurTolerance = -5.0f;        // -0.8f ~ -0.1f 0.25f
	m_UpsampleTolerance = -0.7f;    // -12.0f ~ -0.1f, 0.5f
	m_RejectionFallOff = 2.5f;      // 1 ~ 10 , 0.5
	
	m_Accentuation = 0.1f; // 0.1f~1.0f
	m_ScreenSpaceDiameter = 10.0f; // 0.0f ~ 200.0f, 10.0f

	m_HierarchyDepth = 3u; // 1 ~ 4
	m_eQuality = QualityLevel::kHigh;

	m_SampleThickness[0] = sqrt(1.0f - 0.2f * 0.2f);
	m_SampleThickness[1] = sqrt(1.0f - 0.4f * 0.4f);
	m_SampleThickness[2] = sqrt(1.0f - 0.6f * 0.6f);
	m_SampleThickness[3] = sqrt(1.0f - 0.8f * 0.8f);
	m_SampleThickness[4] = sqrt(1.0f - 0.2f * 0.2f - 0.2f * 0.2f);
	m_SampleThickness[5] = sqrt(1.0f - 0.2f * 0.2f - 0.4f * 0.4f);
	m_SampleThickness[6] = sqrt(1.0f - 0.2f * 0.2f - 0.6f * 0.6f);
	m_SampleThickness[7] = sqrt(1.0f - 0.2f * 0.2f - 0.8f * 0.8f);
	m_SampleThickness[8] = sqrt(1.0f - 0.4f * 0.4f - 0.4f * 0.4f);
	m_SampleThickness[9] = sqrt(1.0f - 0.4f * 0.4f - 0.6f * 0.6f);
	m_SampleThickness[10] = sqrt(1.0f - 0.4f * 0.4f - 0.8f * 0.8f);
	m_SampleThickness[11] = sqrt(1.0f - 0.6f * 0.6f - 0.6f * 0.6f);
}

void SSAOPass::Reset()
{

}

void SSAOPass::RenderWindow()
{
	/*
	bool m_bEnable;
	bool m_bDebugDraw;
	bool m_bAsyncCompute;
	bool m_bComputeLinearZ;

	float m_NoiseFilterTolerance; // // -8.0f ~ 0.0f, 0.25f
	float m_BlurTolerance; // -0.8f ~ -0.1f 0.25f
	float m_UpsampleTolerance; // -12.0f ~ -0.1f, 0.5f
	float m_RejectionFallOff; // 1 ~ 10 , 0.5
	float m_Accentuation; // 0.1f~1.0f
	float m_ScreenSpaceDiameter; // 0.0f ~ 200.0f, 10.0f
	uint32_t m_HierarchyDepth; // 1 ~ 4

	QualityLevel m_eQuality; { kVeryLow = 0, kLow, kMedium, kHigh, kVeryHigh, kCount };
	*/
	ImGui::BeginChild("Test1", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

	ImGui::Checkbox("SSAO Enable", &m_bEnable);
	ImGui::Checkbox("SSAO Enable", &m_bDebugDraw);
	ImGui::Checkbox("SSAO Enable", &m_bAsyncCompute);
	ImGui::Checkbox("SSAO Enable", &m_bComputeLinearZ);

	ImGui::SliderFloat("NoiseFilter Tolerance", &m_NoiseFilterTolerance, -8.0f, 0.1f, "%.25f");
	ImGui::SliderFloat("Blur Tolerance", &m_BlurTolerance, -8.0f, -0.1f, "%.25f");
	ImGui::SliderFloat("Upsample Tolerance", &m_UpsampleTolerance, -12.0f, -0.1f, "%.5f");
	ImGui::SliderFloat("Rejection Fall-off", &m_RejectionFallOff, 1.0f, 10.0f, "%.5f");
	ImGui::SliderFloat("Screen-Space Sphere Diameter", &m_ScreenSpaceDiameter, 10.0f, 200.0f, "10.0f");
	ImGui::SliderInt("HierarchyDepth", &m_HierarchyDepth, 1, 4);

	static const char* CurrentLevelLabel = m_QualityLabel[0];

	if (ImGui::BeginCombo("SSAO Quality", CurrentLevelLabel))
	{
		for (size_t i = 0; i < (size_t)QualityLevel::kCount; ++i)
		{
			const bool bSelected = (CurrentLevelLabel == m_QualityLabel[i]);
			if (ImGui::Selectable(m_QualityLabel[i], bSelected))
			{
				m_eQuality = QualityLevel(i);
			}
			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::EndChild();
}

void SSAOPass::Execute(custom::CommandContext& BaseContext)
{
#ifdef _DEBUG
	graphics::InitDebugStartTick();
	float DeltaTime1 = graphics::GetDebugFrameTime();
#endif

	Camera* pMainCamera = BaseContext.GetpMainCamera();

	const Math::Matrix4& ProjectMatrix = pMainCamera->GetProjMatrix();

	float ZCoefficient{ 0.0f };
	{
		const float NearZClip = pMainCamera->GetNearZClip();
		const float FarZClip = pMainCamera->GetFarZClip();
		ZCoefficient = (FarZClip - NearZClip) / NearZClip;
	}

	const size_t CurrentFrameIndex = graphics::GetFrameCount() % device::g_DisplayBufferCount;
	ColorBuffer& LinearDepth = bufferManager::g_LinearDepth[CurrentFrameIndex];
	ColorBuffer& SSAOTarget = bufferManager::g_SSAOFullScreen;
	ColorBuffer& MainRenderTarget = bufferManager::g_SceneColorBuffer;
	DepthBuffer& MainDepthBuffer = bufferManager::g_SceneDepthBuffer;

	BaseContext.PIXBeginEvent(L"4_SSAOPass");

	// Linearlize
	if (!m_bEnable)
	{
		custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

		graphicsContext.TransitionResource(SSAOTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		graphicsContext.ClearColor(SSAOTarget);
		graphicsContext.TransitionResource(SSAOTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		if (m_bComputeLinearZ == false)
		{
			graphicsContext.PIXEndEvent();
			return;
		}

		custom::ComputeContext& computeContext = graphicsContext.GetComputeContext();

		computeContext.SetRootSignature(m_MainRootSignature);

		computeContext.TransitionResource(SSAOTarget, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.SetConstant(0, ZCoefficient);
		computeContext.SetDynamicDescriptor(3, 0, MainDepthBuffer.GetDepthSRV()); // Input : DepthBuffer

		computeContext.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Context.SetDynamicDescriptors(2, 0, 1, &LinearDepth.GetUAV());  
		// Output : g_LinearDepth = 1.0f / (DepthBuffer * ZCoefficient + 1) 
		computeContext.SetDynamicDescriptor(2, 0, LinearDepth.GetUAV());

		computeContext.SetPipelineState(m_LinearizeDepthCS);
		computeContext.Dispatch2D(LinearDepth.GetWidth(), LinearDepth.GetHeight(), 16, 16);

		if (m_bDebugDraw)
		{
			computeContext.TransitionResource(MainRenderTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			computeContext.SetDynamicDescriptors(2, 0, 1, &MainRenderTarget.GetUAV()); // Output(Will be RenderTarget)
			computeContext.SetDynamicDescriptors(3, 0, 1, &LinearDepth.GetSRV());      // Input
			computeContext.SetPipelineState(m_DebugSSAOCS);
			computeContext.Dispatch2D(SSAOTarget.GetWidth(), SSAOTarget.GetHeight());
		}
		computeContext.PIXEndEvent();
		return;
	} // End Linearize

	{
		custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

		graphicsContext.TransitionResource(MainDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		graphicsContext.TransitionResource(SSAOTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		if (m_bAsyncCompute)
		{
			// Flush the ZPrePass and wait for it on the compute queue
			custom::CommandQueue& computeQueue = device::g_commandQueueManager.GetComputeQueue();
			computeQueue.StallForFence(graphicsContext.Flush());
		}
	}

	custom::ComputeContext& computeContext = m_bAsyncCompute ? custom::ComputeContext::Begin(L"Async SSAO") : BaseContext.GetComputeContext();
	computeContext.SetRootSignature(m_MainRootSignature);

	// For naming Memo.
	// Cuda //  Compute Shader // 대응 H/W
	// 연산단위        Thread        Thread              sp or CudaCore
	// HW점유단위     Block         Group                SM

	// Decompressing and Downsampling Buffer
	{
		computeContext.SetConstant(0, ZCoefficient);
		computeContext.SetDynamicDescriptor(3, 0, MainDepthBuffer.GetDepthSRV());

		computeContext.TransitionResource(LinearDepth,                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // LinearDepth
		computeContext.TransitionResource(bufferManager::g_DepthDownsize1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // DS2x
		computeContext.TransitionResource(bufferManager::g_DepthDownsize2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // DS4x
		computeContext.TransitionResource(bufferManager::g_DepthTiled1,    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // DS2xAtlas
		computeContext.TransitionResource(bufferManager::g_DepthTiled2,    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // DS4xAtlas

		D3D12_CPU_DESCRIPTOR_HANDLE DownsizeUAVs[5] =
		{
			LinearDepth.GetUAV(),
			bufferManager::g_DepthDownsize1.GetUAV(),
			bufferManager::g_DepthTiled1.GetUAV(),
			bufferManager::g_DepthDownsize2.GetUAV(),
			bufferManager::g_DepthTiled2.GetUAV()
		};
		computeContext.SetDynamicDescriptors(2, 0, 5, DownsizeUAVs);

		computeContext.SetPipelineState(m_DepthPrepareCS_16);
		computeContext.Dispatch2D
		(
			bufferManager::g_DepthTiled2.GetWidth() * NUM_GROUP_THREAD_WIDTH, 
			bufferManager::g_DepthTiled2.GetHeight() * NUM_GROUP_THREAD_HEIGHT
		);
	}
	{
		if (2 < m_HierarchyDepth)
		{
			float temp1 = 1.0f / bufferManager::g_DepthDownsize2.GetWidth();
			float temp2 = 1.0f / bufferManager::g_DepthDownsize2.GetHeight();

			computeContext.SetConstants(0, *reinterpret_cast<UINT*>(&temp1), *reinterpret_cast<UINT*>(&temp2)); // (b0)

			computeContext.TransitionResource(bufferManager::g_DepthDownsize2, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); // (t0)

			computeContext.TransitionResource(bufferManager::g_DepthDownsize3, D3D12_RESOURCE_STATE_UNORDERED_ACCESS); // (u0)
			computeContext.TransitionResource(bufferManager::g_DepthDownsize4, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext.TransitionResource(bufferManager::g_DepthTiled3, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext.TransitionResource(bufferManager::g_DepthTiled4, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			D3D12_CPU_DESCRIPTOR_HANDLE DownsizeAgainUAVs[4] = 
			{ 
				bufferManager::g_DepthDownsize3.GetUAV(),
				bufferManager::g_DepthDownsize4.GetUAV(),
				bufferManager::g_DepthTiled3.GetUAV(),
				bufferManager::g_DepthTiled4.GetUAV()
			};

			computeContext.SetDynamicDescriptors(2, 0, 4, DownsizeAgainUAVs);
			computeContext.SetDynamicDescriptors(3, 0, 1, &bufferManager::g_DepthDownsize2.GetSRV());
			computeContext.SetPipelineState(m_DepthPrepareCS_64);
			computeContext.Dispatch2D(bufferManager::g_DepthTiled4.GetWidth() * NUM_GROUP_THREAD_WIDTH, bufferManager::g_DepthTiled4.GetHeight() * NUM_GROUP_THREAD_HEIGHT);
		}
	} // End Decompressing and Downsampling Buffer

	// Analyzing Depth Range
	{
		computeContext.TransitionResource(bufferManager::g_AOMerged1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_AOMerged2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_AOMerged3, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_AOMerged4, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_AOHighQuality1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_AOHighQuality2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_AOHighQuality3, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_AOHighQuality4, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_DepthTiled1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DepthTiled2, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DepthTiled3, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DepthTiled4, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DepthDownsize1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DepthDownsize2, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DepthDownsize3, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DepthDownsize4, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		const float ZNear = ProjectMatrix.GetX().GetX();
		const float FovTangent = 1.0f / ZNear;

		// Phase 2:  Render SSAO for each sub-tile
		if (3 < m_HierarchyDepth)
		{
			computeContext.SetPipelineState(m_RenderWithWideSamplingCS);
			ComputeAO(computeContext, bufferManager::g_AOMerged4, bufferManager::g_DepthTiled4, FovTangent);
			if (QualityLevel::kLow <= m_eQuality)
			{
				computeContext.SetPipelineState(m_RenderWithDepthArrayCS);
				ComputeAO(computeContext, bufferManager::g_AOHighQuality4, bufferManager::g_DepthDownsize4, FovTangent);
			}
		}
		if (2 < m_HierarchyDepth)
		{
			computeContext.SetPipelineState(m_RenderWithWideSamplingCS);
			ComputeAO(computeContext, bufferManager::g_AOMerged3, bufferManager::g_DepthTiled3, FovTangent);
			if (QualityLevel::kMedium <= m_eQuality)
			{
				computeContext.SetPipelineState(m_RenderWithDepthArrayCS);
				ComputeAO(computeContext, bufferManager::g_AOHighQuality3, bufferManager::g_DepthDownsize3, FovTangent);
			}
		}
		if (1 < m_HierarchyDepth)
		{
			computeContext.SetPipelineState(m_RenderWithWideSamplingCS);
			ComputeAO(computeContext, bufferManager::g_AOMerged2, bufferManager::g_DepthTiled2, FovTangent);
			if (QualityLevel::kHigh <= m_eQuality)
			{
				computeContext.SetPipelineState(m_RenderWithDepthArrayCS);
				ComputeAO(computeContext, bufferManager::g_AOHighQuality2, bufferManager::g_DepthDownsize2, FovTangent);
			}
		}

		{
			computeContext.SetPipelineState(m_RenderWithWideSamplingCS);
			ComputeAO(computeContext, bufferManager::g_AOMerged1, bufferManager::g_DepthTiled1, FovTangent);
			if (QualityLevel::kVeryHigh <= m_eQuality)
			{
				computeContext.SetPipelineState(m_RenderWithDepthArrayCS);
				ComputeAO(computeContext, bufferManager::g_AOHighQuality1, bufferManager::g_DepthDownsize1, FovTangent);
			}
		}
	}// End Analyzing Depth Range

	// Blur and Upsampling
	// Iteratively blur and Upsamling, combining each result.
	{
		ColorBuffer* InterleavedAO = &bufferManager::g_AOMerged4;

		// 1 / 16 -> 1 / 8
		if (3 < m_HierarchyDepth)
		{
			BlurAndUpsampling
			(
				computeContext, bufferManager::g_AOSmooth3, bufferManager::g_DepthDownsize3, bufferManager::g_DepthDownsize4,
				InterleavedAO, (QualityLevel::kLow <= m_eQuality) ? &bufferManager::g_AOHighQuality4 : nullptr, & bufferManager::g_AOMerged3
			);

			InterleavedAO = &bufferManager::g_AOSmooth3;
		}
		else
		{
			InterleavedAO = &bufferManager::g_AOMerged3;
		}

		// 1 / 8 -> 1 / 4
		if (2 < m_HierarchyDepth)
		{
			BlurAndUpsampling
			(
				computeContext, bufferManager::g_AOSmooth2, bufferManager::g_DepthDownsize2, bufferManager::g_DepthDownsize3,
				InterleavedAO, (QualityLevel::kMedium <= m_eQuality) ? &bufferManager::g_AOHighQuality3 : nullptr, &bufferManager::g_AOMerged2
			);

			InterleavedAO = &bufferManager::g_AOSmooth2;
		}
		else
		{
			InterleavedAO = &bufferManager::g_AOMerged2;
		}

		// 1 / 4 -> 1 / 2
		if (1 < m_HierarchyDepth)
		{
			BlurAndUpsampling
			(
				computeContext, bufferManager::g_AOSmooth1, bufferManager::g_DepthDownsize1, bufferManager::g_DepthDownsize2,
				InterleavedAO, (QualityLevel::kHigh <= m_eQuality) ? &bufferManager::g_AOHighQuality2 : nullptr, &bufferManager::g_AOMerged1
			);

			InterleavedAO = &bufferManager::g_AOSmooth1;
		}
		else
		{
			InterleavedAO = &bufferManager::g_AOMerged1;
		}

		// 1 / 2 -> Original
		BlurAndUpsampling
		(
			computeContext, bufferManager::g_SSAOFullScreen, LinearDepth, bufferManager::g_DepthDownsize1,
			InterleavedAO, (QualityLevel::kVeryHigh <= m_eQuality) ? &bufferManager::g_AOHighQuality1 : nullptr, nullptr
		);
	}// End Blur and Upsampling

	if (m_bAsyncCompute)
	{
		computeContext.Finish();
	}
	else
	{
		computeContext.PIXEndEvent();
	}

	if (m_bDebugDraw)
	{
		if (m_bAsyncCompute)
		{
			custom::CommandQueue& graphicsQueue = device::g_commandQueueManager.GetGraphicsQueue();

			graphicsQueue.StallForProducer(device::g_commandQueueManager.GetComputeQueue());
		}

		custom::ComputeContext& NewComputeContext = BaseContext.GetComputeContext();

		NewComputeContext.PIXBeginEvent(L"SSAO_Debug_Draw");

		NewComputeContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		NewComputeContext.TransitionResource(bufferManager::g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		NewComputeContext.SetRootSignature(m_MainRootSignature);
		NewComputeContext.SetPipelineState(m_DebugSSAOCS);
		NewComputeContext.SetDynamicDescriptors(2, 0, 1, &bufferManager::g_SceneColorBuffer.GetUAV());
		NewComputeContext.SetDynamicDescriptors(3, 0, 1, &bufferManager::g_SSAOFullScreen.GetSRV());
		NewComputeContext.Dispatch2D(bufferManager::g_SSAOFullScreen.GetWidth(), bufferManager::g_SSAOFullScreen.GetHeight());

		NewComputeContext.PIXEndEvent();
	}

#ifdef _DEBUG
	float DeltaTime2 = graphics::GetDebugFrameTime();
	m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}

void SSAOPass::BlurAndUpsampling
(
	custom::ComputeContext& computeContext,
	ColorBuffer& Result, ColorBuffer& HighResolutionDepth, ColorBuffer& LowResolutionDepth,
	ColorBuffer* InterleavedAO, ColorBuffer* HighQualityAO, ColorBuffer* HighResolutionAO
)
{
	{
		ComputePSO* computeShader{ nullptr };

		if (HighResolutionAO == nullptr)
		{
			computeShader = (HighQualityAO) ? &m_BlurUpSampleFinalWithCombineLowerResolutionCS : &m_BlurUpSampleFinalWithNoneCS;
		}
		else
		{
			computeShader = (HighQualityAO) ? &m_BlurUpSampleBlendWithBothCS : &m_BlurUpSamleBlendWithHighResolutionCS;
		}

		computeContext.SetPipelineState(*computeShader);
	}

	size_t HighLowWidthHeight[] = { LowResolutionDepth.GetWidth(), LowResolutionDepth.GetHeight(), HighResolutionDepth.GetWidth(), HighResolutionDepth.GetHeight() };

	const float kBlurTolerance = powf(1.0f - (powf(10.0f, m_BlurTolerance) * bufferManager::g_SceneColorBuffer.GetWidth() / (float)HighLowWidthHeight[0]), 2.0f);
	const float kUpsampleTolerance = powf(10.0f, m_UpsampleTolerance);
	const float kNoiseFilterWeight = 1.0f / (powf(10.0f, m_NoiseFilterTolerance) + kUpsampleTolerance);
	
	__declspec(align(16)) float cbData[] = 
	{
			1.0f / HighLowWidthHeight[0], 1.0f / HighLowWidthHeight[1], 1.0f / HighLowWidthHeight[2], 1.0f / HighLowWidthHeight[3],
			kNoiseFilterWeight, 1920.0f / (float)HighLowWidthHeight[0], kBlurTolerance, kUpsampleTolerance
	};

	computeContext.TransitionResource(Result, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	computeContext.TransitionResource(LowResolutionDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	computeContext.TransitionResource(HighResolutionDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	computeContext.SetDynamicDescriptor(2, 0, Result.GetUAV());
	computeContext.SetDynamicDescriptor(3, 0, LowResolutionDepth.GetSRV());
	computeContext.SetDynamicDescriptor(3, 1, HighResolutionDepth.GetSRV());

	if (InterleavedAO != nullptr)
	{
		computeContext.TransitionResource(*InterleavedAO, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.SetDynamicDescriptor(3, 2, InterleavedAO->GetSRV());
	}
	if (HighQualityAO != nullptr)
	{
		computeContext.TransitionResource(*HighQualityAO, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.SetDynamicDescriptor(3, 3, HighQualityAO->GetSRV());
	}
	if (HighResolutionAO != nullptr)
	{
		computeContext.TransitionResource(*HighResolutionAO, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.SetDynamicDescriptor(3, 4, HighResolutionAO->GetSRV());
	}

	computeContext.Dispatch2D(HighLowWidthHeight[2] + 2, HighLowWidthHeight[3] + 2, 16, 16);
}

void SSAOPass::ComputeAO(custom::ComputeContext& Context, ColorBuffer& Result, ColorBuffer& _DepthBuffer, const float TangentHalfFOVHorizontal)
{
	size_t BufferWidth = _DepthBuffer.GetWidth();
	size_t BufferHeight = _DepthBuffer.GetHeight();
	size_t ArrayCount = _DepthBuffer.GetDepth();

	// Here we compute multipliers that convert the center depth value into (the reciprocal of)
	// sphere thicknesses at each sample location.  
	// This assumes a maximum sample radius of 5 units,
	// but since a sphere has no thickness at its extent, we don't need to sample that far out.
	// Only samples whole integer offsets with distance less than 25 are used.  
	// This means that there is no sample at (3, 4).
	// because its distance is exactly 25 (and has a thickness of 0.)

	// The shaders are set up to sample a circular region within a 5-pixel radius.
	// const float ScreenspaceDiameter = 10.0f;

	// SphereDiameter = CenterDepth * ThicknessMultiplier.  
	// This will compute the thickness of a sphere centered at a specific depth.
	// The ellipsoid scale can stretch a sphere into an ellipsoid, which changes the characteristics of the AO.

	// Tangent Half Fov Horizontal:  Radius of sphere in depth units if its center lies at Z = 1
	// ScreenspaceDiameter: Diameter of sample sphere in pixel units
	// ScreenspaceDiameter / BufferWidth:  Ratio of the screen width that the sphere actually covers
	// Note about the "2.0f * ":  Diameter = 2 * Radius
	float ThicknessMultiplier = 2.0f * TangentHalfFOVHorizontal * m_ScreenSpaceDiameter / BufferWidth;

	if (ArrayCount == 1)
	{
		ThicknessMultiplier *= 2.0f;
	}

	// This will m_Transform a depth value from [0, thickness] to [0, 1].
	float InverseRangeFactor = 1.0f / ThicknessMultiplier;

	__declspec(align(16)) float SsaoCB[28];
	{
		// The thicknesses are smaller for all off-center samples of the sphere.  
		// Compute thicknesses relative to the center sample.
		SsaoCB[0] = InverseRangeFactor / m_SampleThickness[0];
		SsaoCB[1] = InverseRangeFactor / m_SampleThickness[1];
		SsaoCB[2] = InverseRangeFactor / m_SampleThickness[2];
		SsaoCB[3] = InverseRangeFactor / m_SampleThickness[3];
		SsaoCB[4] = InverseRangeFactor / m_SampleThickness[4];
		SsaoCB[5] = InverseRangeFactor / m_SampleThickness[5];
		SsaoCB[6] = InverseRangeFactor / m_SampleThickness[6];
		SsaoCB[7] = InverseRangeFactor / m_SampleThickness[7];
		SsaoCB[8] = InverseRangeFactor / m_SampleThickness[8];
		SsaoCB[9] = InverseRangeFactor / m_SampleThickness[9];
		SsaoCB[10] = InverseRangeFactor / m_SampleThickness[10];
		SsaoCB[11] = InverseRangeFactor / m_SampleThickness[11];

		// These are the weights that are multiplied against the samples because not all samples are
		// equally important.  The farther the sample is from the center location, the less they matter.
		// We use the thickness of the sphere to determine the weight.  The scalars in front are the number
		// of samples with this weight because we sum the samples together before multiplying by the weight,
		// so as an aggregate all of those samples matter more.  After generating this table, the weights
		// are normalized.
		SsaoCB[12] = 4.0f * m_SampleThickness[0];    // Axial
		SsaoCB[13] = 4.0f * m_SampleThickness[1];    // Axial
		SsaoCB[14] = 4.0f * m_SampleThickness[2];    // Axial
		SsaoCB[15] = 4.0f * m_SampleThickness[3];    // Axial
		SsaoCB[16] = 4.0f * m_SampleThickness[4];    // Diagonal
		SsaoCB[17] = 8.0f * m_SampleThickness[5];    // L-shaped
		SsaoCB[18] = 8.0f * m_SampleThickness[6];    // L-shaped
		SsaoCB[19] = 8.0f * m_SampleThickness[7];    // L-shaped
		SsaoCB[20] = 4.0f * m_SampleThickness[8];    // Diagonal
		SsaoCB[21] = 8.0f * m_SampleThickness[9];    // L-shaped
		SsaoCB[22] = 8.0f * m_SampleThickness[10];   // L-shaped
		SsaoCB[23] = 4.0f * m_SampleThickness[11];   // Diagonal

	//#define SAMPLE_EXHAUSTIVELY
	// If we aren't using all of the samples, delete their weights before we normalize.
#ifndef SAMPLE_EXHAUSTIVELY
		SsaoCB[12] = 0.0f;
		SsaoCB[14] = 0.0f;
		SsaoCB[17] = 0.0f;
		SsaoCB[19] = 0.0f;
		SsaoCB[21] = 0.0f;
#endif

		// Normalize the weights by dividing by the sum of all weights
		float totalWeight = 0.0f;

		for (size_t i = 12; i < 24; ++i)
		{
			totalWeight += SsaoCB[i];
		}

		for (size_t i = 12; i < 24; ++i)
		{
			SsaoCB[i] /= totalWeight;
		}

		SsaoCB[24] = 1.0f / BufferWidth;
		SsaoCB[25] = 1.0f / BufferHeight;
		SsaoCB[26] = 1.0f / -m_RejectionFallOff;
		SsaoCB[27] = 1.0f / (1.0f + m_Accentuation);
	}
	Context.SetDynamicConstantBufferView(1, sizeof(SsaoCB), SsaoCB);
	Context.SetDynamicDescriptor(2, 0, Result.GetUAV());
	Context.SetDynamicDescriptor(3, 0, _DepthBuffer.GetSRV());

	if (ArrayCount == 1)
	{
		Context.Dispatch2D(BufferWidth, BufferHeight, 16, 16);
	}
	else
	{
		Context.Dispatch3D(BufferWidth, BufferHeight, ArrayCount, 8, 8, 1);
	}
}