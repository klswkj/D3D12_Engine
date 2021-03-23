#include "stdafx.h"
#include "SSAOPass.h"
#include "PreMadePSO.h"
#include "Camera.h"
#include "CommandContext.h"
#include "ComputeContext.h"
#include "CommandContextManager.h"
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

#define NUM_GROUP_THREAD_WIDTH 8ull
#define NUM_GROUP_THREAD_HEIGHT 8ull

// Linearize
// Screen-Space reconstruct
// Random sample Generation

SSAOPass* SSAOPass::s_pSSAOPass = nullptr;

// TODO 1 : Recude number of RootSignature.

SSAOPass::SSAOPass(std::string name)
	:
	ID3D12ScreenPass(name),
	m_FovTangent(0.0f),
	m_bEnable(true),
	m_bDebugDraw(false),
	m_bAsyncCompute(false),
	m_bComputeLinearZ(true),
	m_CS({})
{
	ASSERT(s_pSSAOPass == nullptr);
	s_pSSAOPass = this;

	{
		m_MainRootSignature.Reset(4, 2);
		m_MainRootSignature.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
		m_MainRootSignature.InitStaticSampler(1, premade::g_SamplerLinearBorderDesc);
		m_MainRootSignature[0].InitAsConstants(0, 4);
		m_MainRootSignature[1].InitAsConstantBuffer(1);
		m_MainRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 5);
		m_MainRootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5);
		m_MainRootSignature.Finalize(L"SSAO_RS");

		m_LinearizeDepthSignature.Reset(3, 0);
		m_LinearizeDepthSignature[0].InitAsConstants(0, 1);
		m_LinearizeDepthSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		m_LinearizeDepthSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
		m_LinearizeDepthSignature.Finalize(L"m_LinearizeDepth");
	}
#define CreatePSO( ObjName, ShaderByteCode )                           \
    ObjName.SetRootSignature(m_MainRootSignature);                     \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize(L#ObjName)

	{
		CreatePSO(m_DepthPrepareCS_16, g_pAoPrepareDepthBuffers1CS);
		CreatePSO(m_DepthPrepareCS_64, g_pAoPrepareDepthBuffers2CS);
		CreatePSO(m_RenderWithWideSamplingCS, g_pAoCompute1CS);
		CreatePSO(m_RenderWithDepthArrayCS, g_pAoCompute2CS);
	}
	{
		CreatePSO(m_BlurUpSampleFinalWithNoneCS,                   g_pAoBlurUpsampleCS);
		CreatePSO(m_BlurUpSampleFinalWithCombineLowerResolutionCS, g_pAoBlurUpsamplePreMinCS);
		CreatePSO(m_BlurUpSampleBlendWithHighResolutionCS,         g_pAoBlurUpsampleBlendOutCS);
		CreatePSO(m_BlurUpSampleBlendWithBothCS,                   g_pAoBlurUpsamplePreMinBlendOutCS);
	}
	{
		// CreatePSO(m_LinearizeDepthCS, g_pLinearizeDepthCS);
		m_LinearizeDepthCS.SetRootSignature(m_LinearizeDepthSignature);
		m_LinearizeDepthCS.SetComputeShader(g_pLinearizeDepthCS, sizeof(g_pLinearizeDepthCS));
		m_LinearizeDepthCS.Finalize(L"m_LinearizeDepthCS");

		// CreatePSO(m_DebugSSAOCS, g_pDebugSSAOCS);
		m_DebugSSAOCS.SetRootSignature(m_LinearizeDepthSignature);
		m_DebugSSAOCS.SetComputeShader(g_pDebugSSAOCS, sizeof(g_pDebugSSAOCS));
		m_DebugSSAOCS.Finalize(L"m_LinearizeDepthCS");
	}
#undef CreatePSO

	m_NoiseFilterTolerance = -3.0f; // -8.0f ~ 0.0f, 0.25f
	m_BlurTolerance        = -5.0f; // -0.8f ~ -0.1f 0.25f
	m_UpsampleTolerance    = -0.7f; // -12.0f ~ -0.1f, 0.5f
	m_RejectionFallOff     =  2.5f; // 1 ~ 10 , 0.5
	
	m_Accentuation = 0.1f; // 0.1f~1.0f
	m_ScreenSpaceDiameter = 10.0f; // 0.0f ~ 200.0f, 10.0f

	m_HierarchyDepth = 3u; // 1 ~ 4
	m_eQuality = QualityLevel::kHigh;

	m_SampleThickness[0]  = sqrt(1.0f - 0.2f * 0.2f);
	m_SampleThickness[1]  = sqrt(1.0f - 0.4f * 0.4f);
	m_SampleThickness[2]  = sqrt(1.0f - 0.6f * 0.6f);
	m_SampleThickness[3]  = sqrt(1.0f - 0.8f * 0.8f);
	m_SampleThickness[4]  = sqrt(1.0f - 0.2f * 0.2f - 0.2f * 0.2f);
	m_SampleThickness[5]  = sqrt(1.0f - 0.2f * 0.2f - 0.4f * 0.4f);
	m_SampleThickness[6]  = sqrt(1.0f - 0.2f * 0.2f - 0.6f * 0.6f);
	m_SampleThickness[7]  = sqrt(1.0f - 0.2f * 0.2f - 0.8f * 0.8f);
	m_SampleThickness[8]  = sqrt(1.0f - 0.4f * 0.4f - 0.4f * 0.4f);
	m_SampleThickness[9]  = sqrt(1.0f - 0.4f * 0.4f - 0.6f * 0.6f);
	m_SampleThickness[10] = sqrt(1.0f - 0.4f * 0.4f - 0.8f * 0.8f);
	m_SampleThickness[11] = sqrt(1.0f - 0.6f * 0.6f - 0.6f * 0.6f);

	if (!m_CS.DebugInfo)
	{
		::InitializeCriticalSection(&m_CS);
	}
}

SSAOPass::~SSAOPass()
{
	if(m_CS.DebugInfo)
	{
		::DeleteCriticalSection(&m_CS);
	}
}

void SSAOPass::Reset()
{
}

void SSAOPass::RenderWindow()
{
	ImGui::BeginChild("Test1", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

	ImGui::Checkbox("SSAO Enable", &m_bEnable);
	ImGui::Checkbox("SSAO Enable", &m_bDebugDraw);
	ImGui::Checkbox("SSAO Enable", &m_bAsyncCompute);
	ImGui::Checkbox("SSAO Enable", &m_bComputeLinearZ);

	ImGui::SliderFloat("NoiseFilter Tolerance",        &m_NoiseFilterTolerance, -8.0f, 0.1f, "%.25f");
	ImGui::SliderFloat("Blur Tolerance",               &m_BlurTolerance,       -8.0f, -0.1f, "%.25f");
	ImGui::SliderFloat("Upsample Tolerance",           &m_UpsampleTolerance,  -12.0f, -0.1f, "%.5f");
	ImGui::SliderFloat("Rejection Fall-off",           &m_RejectionFallOff,     1.0f, 10.0f, "%.5f");
	ImGui::SliderFloat("Screen-Space Sphere Diameter", &m_ScreenSpaceDiameter, 10.0f, 200.0f, "10.0f");
	ImGui::SliderInt  ("HierarchyDepth",               &m_HierarchyDepth,          1,     4);

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

void SSAOPass::ExecutePass()
{
#ifdef _DEBUG
	graphics::InitDebugStartTick();
	float DeltaTime1 = graphics::GetDebugFrameTime();
#endif

	Camera* pMainCamera = CommandContextManager::GetMainCamera();

	float ZCoefficient = 0.0f;
	{
		const float NearZClip = pMainCamera->GetNearZClip();
		ZCoefficient = (pMainCamera->GetFarZClip() - NearZClip) / NearZClip;
	}

	__declspec(align(16)) float CB_Empty[] =
	{ m_NoiseFilterTolerance, m_BlurTolerance, m_UpsampleTolerance, m_RejectionFallOff };

	const size_t CurrentFrameIndex = graphics::GetFrameCount() % device::g_DisplayBufferCount;
	ColorBuffer& LinearDepth       = bufferManager::g_LinearDepth[CurrentFrameIndex]; // // Normalized planar distance (0 at eye, 1 at far plane) computed from the SceneDepthBuffer

	// Linearlize
	if (!m_bEnable)
	{
		custom::GraphicsContext& graphicsContext = custom::GraphicsContext::Begin(1u);
		graphicsContext.SetResourceTransitionBarrierIndex(0u);

		graphicsContext.PIXBeginEvent(L"4_SSAOPass", 0u);
		graphicsContext.PIXBeginEvent(L"SSAO_Linearize", 0u);

		graphicsContext.TransitionResource(bufferManager::g_SSAOFullScreen, D3D12_RESOURCE_STATE_RENDER_TARGET, 0U);
		graphicsContext.SubmitResourceBarriers(0u);
		graphicsContext.ClearColor(bufferManager::g_SSAOFullScreen, 0u);
		graphicsContext.TransitionResource(bufferManager::g_SSAOFullScreen, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0U);
		graphicsContext.SubmitResourceBarriers(0u);

		if (m_bComputeLinearZ == false)
		{
			graphicsContext.PIXEndEvent(0u); // End SSAO_Linearize
			graphicsContext.PIXEndEvent(0u); // End 4_SSAOPass
			graphicsContext.Finish();
			return;
		}
		
		graphicsContext.Finish();
		
		custom::ComputeContext& computeContext = custom::ComputeContext::Begin(1u);
		computeContext.SetResourceTransitionBarrierIndex(0u);
		/*
		computeContext.SetRootSignature(m_MainRootSignature);
		computeContext.SetPipelineState(m_LinearizeDepthCS);

		computeContext.TransitionResource(LinearDepth,     D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(MainDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// Output : g_LinearDepth = 1.0f / (MainDepthBuffer * ZCoefficient + 1) 
		computeContext.SetConstant(0, *reinterpret_cast<UINT*>(&ZCoefficient));
		computeContext.SetDynamicDescriptor(2, 0, LinearDepth.GetUAV());
		computeContext.SetDynamicDescriptor(3, 0, MainDepthBuffer.GetDepthSRV()); // Input : DepthBuffer
		computeContext.Dispatch2D(LinearDepth.GetWidth(), LinearDepth.GetHeight(), 16, 16);
		*/

		// SetDynamicDescriptor(2, 0) -> GPU Descriptor Handle (at 1 offsetInDescriptorsForDescriptorHeapStart)
		// SetDynamicDescriptor(2, 1) -> GPU Descriptor Handle (at 2 offsetInDescriptorsForDescriptorHeapStart)
		// SetDynamicDescriptor(2, 2) -> GPU Descriptor Handle (at 1 offsetInDescriptorsForDescriptorHeapStart)
		// SetDynamicDescriptor(2, 3) -> GPU Descriptor Handle (at 1 offsetInDescriptorsForDescriptorHeapStart)
		// SetDynamicDescriptor(2, 4) -> GPU Descriptor Handle (at 1 offsetInDescriptorsForDescriptorHeapStart)

		computeContext.PIXBeginEvent(L"SSAO_ComputeLinearZ", 0u);
		computeContext.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_DEPTH_SUBRESOURCE);
		computeContext.SubmitResourceBarriers(0u);

		computeContext.SetRootSignature(m_LinearizeDepthSignature, 0u);
		computeContext.SetPipelineState(m_LinearizeDepthCS, 0u);

		computeContext.SetConstant(0U, *reinterpret_cast<UINT*>(&ZCoefficient), 0u, 0u);
		computeContext.SetDynamicDescriptor(1U, 0U, LinearDepth.GetUAV(), 0u);
		computeContext.SetDynamicDescriptor(2U, 0U, bufferManager::g_SceneDepthBuffer.GetDepthSRV(), 0u); // Input : DepthBuffer
		computeContext.Dispatch2D(LinearDepth.GetWidth(), LinearDepth.GetHeight(), 16ul, 16ul, 0u);
		computeContext.PIXEndEvent(0u); // End SSAO_ComputeLinearZ

		if (m_bDebugDraw)
		{
			computeContext.PIXBeginEvent(L"SSAO_DebugDraw", 0u);
			computeContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
			computeContext.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
			computeContext.SubmitResourceBarriers(0u);
			computeContext.SetRootSignature(m_LinearizeDepthSignature, 0u);
			computeContext.SetPipelineState(m_DebugSSAOCS, 0u);
			computeContext.SetDynamicDescriptors(1U, 0U, 1U, &bufferManager::g_SceneColorBuffer.GetUAV(), 0u); // Output(Will be RenderTarget)
			computeContext.SetDynamicDescriptors(2U, 0U, 1U, &LinearDepth.GetSRV(), 0u);      // Input
			computeContext.Dispatch2D(bufferManager::g_SSAOFullScreen.GetWidth(), bufferManager::g_SSAOFullScreen.GetHeight(), 0u);
			computeContext.PIXEndEvent(0); // End SSAO_DebugDraw

			custom::GraphicsContext& LastGraphicsContext = custom::GraphicsContext::GetRecentContext();
			device::g_commandContextManager.FreeContext(&LastGraphicsContext);
		}

		computeContext.PIXEndEvent(0u); // End SSAO_Linearize
		computeContext.PIXEndEvent(0u); // End 4_SSAOPass
		computeContext.WaitLastExecuteGPUSide(graphicsContext);
		computeContext.Finish();
		return;
	} // End Linearize

	// TODO 0 : Transition Resource Barrier Optimization.
	custom::ComputeContext& computeContext = custom::ComputeContext::Begin(1u);
	computeContext.SetResourceTransitionBarrierIndex(0u);
	computeContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 
		D3D12_RESOURCE_BARRIER_DEPTH_SUBRESOURCE);
	computeContext.TransitionResource(bufferManager::g_SSAOFullScreen, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
	computeContext.SubmitResourceBarriers(0u);

	if (m_bAsyncCompute)
	{
		// Execute the ZPrePass and wait for it on the compute queue
		custom::CommandQueue& ComputeQueue  = device::g_commandQueueManager.GetComputeQueue();
		custom::CommandQueue& GraphicsQueue = device::g_commandQueueManager.GetGraphicsQueue();
		ASSERT(ComputeQueue.WaitCommandQueueCompletedGPUSide(GraphicsQueue));
	}
	
	computeContext.PIXBeginEvent(m_bAsyncCompute? (L"4_Async Compute SSAO") : (L"4_Sync Compute SSAO"), 0);
	computeContext.SetRootSignature(m_MainRootSignature, 0u);

	// g_DepthDownsize1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // DS2x
	// g_DepthDownsize2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // DS4x
	// g_DepthTiled1,    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // DS2xAtlas
	// g_DepthTiled2,    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);  // DS4xAtlas
	// CL #0
	{
		computeContext.PIXBeginEvent(L"Decompressing and Downsampling Buffer", 0u);
		{
			computeContext.SetConstant(0U, *reinterpret_cast<UINT*>(&ZCoefficient), 0U, 0u);
			computeContext.SetDynamicDescriptor(3U, 0U, bufferManager::g_SceneDepthBuffer.GetDepthSRV(), 0u);

			computeContext.TransitionResource(LinearDepth,                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);  // LinearDepth
			computeContext.TransitionResource(bufferManager::g_DepthDownsize1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);  // DS2x
			computeContext.TransitionResource(bufferManager::g_DepthDownsize2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);  // DS4x
			computeContext.TransitionResource(bufferManager::g_DepthTiled1,    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);  // DS2xAtlas
			computeContext.TransitionResource(bufferManager::g_DepthTiled2,    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);  // DS4xAtlas
			computeContext.SubmitResourceBarriers(0u);

			D3D12_CPU_DESCRIPTOR_HANDLE DownsizeUAVs[5] =
			{
				LinearDepth.GetUAV(),
				bufferManager::g_DepthDownsize1.GetUAV(),
				bufferManager::g_DepthDownsize2.GetUAV(),
				bufferManager::g_DepthTiled1.GetUAV(),
				bufferManager::g_DepthTiled2.GetUAV()
			};

			computeContext.SetDynamicDescriptors(2U, 0U, (UINT)_countof(DownsizeUAVs), DownsizeUAVs, 0u);
			computeContext.SetPipelineState(m_DepthPrepareCS_16, 0u);
			computeContext.Dispatch2D
			(
				(uint64_t)bufferManager::g_DepthTiled2.GetWidth()  * NUM_GROUP_THREAD_WIDTH,
				(uint64_t)bufferManager::g_DepthTiled2.GetHeight() * NUM_GROUP_THREAD_HEIGHT,
				8ul, 8ul, 
				0u
			);

			computeContext.ExecuteCommands(0u, false, true);
		}
		{
			if (2 < m_HierarchyDepth)
			{
				float temp[2] = { 1.0f / bufferManager::g_DepthDownsize2.GetWidth() , 1.0f / bufferManager::g_DepthDownsize2.GetHeight() };

				computeContext.SetConstants(0U, *reinterpret_cast<UINT*>(&temp[0]), *reinterpret_cast<UINT*>(&temp[1]), 0u); // (b0)

				computeContext.TransitionResource(bufferManager::g_DepthDownsize2, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U); // (t0)
				computeContext.TransitionResource(bufferManager::g_DepthDownsize3, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U); // (u0)
				computeContext.TransitionResource(bufferManager::g_DepthDownsize4, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
				computeContext.TransitionResource(bufferManager::g_DepthTiled3,    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
				computeContext.TransitionResource(bufferManager::g_DepthTiled4,    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
				computeContext.SubmitResourceBarriers(0u);

				D3D12_CPU_DESCRIPTOR_HANDLE DownsizeAgainUAVs[4] =
				{
					bufferManager::g_DepthDownsize3.GetUAV(),
					bufferManager::g_DepthDownsize4.GetUAV(),
					bufferManager::g_DepthTiled3.GetUAV(),
					bufferManager::g_DepthTiled4.GetUAV()
				};

				computeContext.SetDynamicDescriptors(2U, 0U, (UINT)_countof(DownsizeAgainUAVs), DownsizeAgainUAVs, 0u);
				computeContext.SetDynamicDescriptors(3U, 0U, 1U, &bufferManager::g_DepthDownsize2.GetSRV(), 0u);
				computeContext.SetPipelineState(m_DepthPrepareCS_64, 0u);
				computeContext.Dispatch2D
				(
					(uint64_t)bufferManager::g_DepthTiled4.GetWidth()  * NUM_GROUP_THREAD_WIDTH, 
					(uint64_t)bufferManager::g_DepthTiled4.GetHeight() * NUM_GROUP_THREAD_HEIGHT, 
					8ul, 8ul,
					0u
				);
			}
		}
		computeContext.PIXEndEvent(0u); // End Decompressing and Downsampling Buffer
		computeContext.ExecuteCommands(0u, false, true);
	}

	// TODO 0 : 여기 MultiThreading 할 수 있을 듯.
	// Analyzing Depth Range
	{
		computeContext.PIXBeginEvent(L"Analyzing Depth Range", 0u);

		computeContext.TransitionResource(bufferManager::g_AOMerged1,      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_AOMerged2,      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_AOMerged3,      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_AOMerged4,      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_AOHighQuality1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_AOHighQuality2, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_AOHighQuality3, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_AOHighQuality4, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_DepthTiled1,    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		computeContext.TransitionResource(bufferManager::g_DepthTiled2,    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		computeContext.TransitionResource(bufferManager::g_DepthTiled3,    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		computeContext.TransitionResource(bufferManager::g_DepthTiled4,    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		computeContext.TransitionResource(bufferManager::g_DepthDownsize1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
		computeContext.TransitionResource(bufferManager::g_DepthDownsize2, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
		computeContext.TransitionResource(bufferManager::g_DepthDownsize3, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
		computeContext.TransitionResource(bufferManager::g_DepthDownsize4, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
		computeContext.SubmitResourceBarriers(0u);

		const Math::Matrix4& ProjectMatrix = pMainCamera->GetProjMatrix();
		const float ZNear = ProjectMatrix.GetX().GetX();
		const float FovTangent = 1.0f / ZNear;

		// Phase 2:  Render SSAO for each sub-tile
		if (3 < m_HierarchyDepth)
		{
			computeContext.SetPipelineState(m_RenderWithWideSamplingCS, 0u);
			ComputeAO(computeContext, 0u, bufferManager::g_AOMerged4, bufferManager::g_DepthTiled4, FovTangent);
			if (QualityLevel::kLow <= m_eQuality)
			{
				computeContext.SetPipelineState(m_RenderWithDepthArrayCS, 0u);
				ComputeAO(computeContext, 0u, bufferManager::g_AOHighQuality4, bufferManager::g_DepthDownsize4, FovTangent);
			}
		}
		if (2 < m_HierarchyDepth)
		{
			computeContext.SetPipelineState(m_RenderWithWideSamplingCS, 0u);
			ComputeAO(computeContext, 0u, bufferManager::g_AOMerged3, bufferManager::g_DepthTiled3, FovTangent);
			if (QualityLevel::kMedium <= m_eQuality)
			{
				computeContext.SetPipelineState(m_RenderWithDepthArrayCS, 0u);
				ComputeAO(computeContext, 0u, bufferManager::g_AOHighQuality3, bufferManager::g_DepthDownsize3, FovTangent);
			}
		}
		if (1 < m_HierarchyDepth)
		{
			computeContext.SetPipelineState(m_RenderWithWideSamplingCS, 0u);
			ComputeAO(computeContext, 0u, bufferManager::g_AOMerged2, bufferManager::g_DepthTiled2, FovTangent);
			if (QualityLevel::kHigh <= m_eQuality)
			{
				computeContext.SetPipelineState(m_RenderWithDepthArrayCS, 0);
				ComputeAO(computeContext, 0u, bufferManager::g_AOHighQuality2, bufferManager::g_DepthDownsize2, FovTangent);
			}
		}

		{
			computeContext.SetPipelineState(m_RenderWithWideSamplingCS, 0u);
			ComputeAO(computeContext, 0u, bufferManager::g_AOMerged1, bufferManager::g_DepthTiled1, FovTangent);
			if (QualityLevel::kVeryHigh <= m_eQuality)
			{
				computeContext.SetPipelineState(m_RenderWithDepthArrayCS, 0u);
				ComputeAO(computeContext, 0u, bufferManager::g_AOHighQuality1, bufferManager::g_DepthDownsize1, FovTangent);
			}
		}
		computeContext.PIXEndEvent(0u); // End Analyzing Depth Range
		computeContext.ExecuteCommands(0u, false, true);
	}// End Analyzing Depth Range

	// Blur and Upsampling
	// Iteratively blur and Upsamling, combining each result.
	{
		computeContext.PIXBeginEvent(L"Blur and Upsampling", 0u);
		ColorBuffer* InterleavedAO = &bufferManager::g_AOMerged4;

		// 1 / 16 -> 1 / 8
		if (3 < m_HierarchyDepth)
		{
			BlurAndUpsampling
			(
				computeContext, 0u, bufferManager::g_AOSmooth3, bufferManager::g_DepthDownsize3, bufferManager::g_DepthDownsize4,
				InterleavedAO, (QualityLevel::kLow <= m_eQuality) ? &bufferManager::g_AOHighQuality4 : nullptr, &bufferManager::g_AOMerged3
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
				computeContext, 0u, bufferManager::g_AOSmooth2, bufferManager::g_DepthDownsize2, bufferManager::g_DepthDownsize3,
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
				computeContext, 0u, bufferManager::g_AOSmooth1, bufferManager::g_DepthDownsize1, bufferManager::g_DepthDownsize2,
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
			computeContext, 0u, bufferManager::g_SSAOFullScreen, LinearDepth, bufferManager::g_DepthDownsize1,
			InterleavedAO, (QualityLevel::kVeryHigh <= m_eQuality) ? &bufferManager::g_AOHighQuality1 : nullptr, nullptr
		);

		computeContext.PIXEndEvent(0u); // End Blur and Upsampling
	}// End Blur and Upsampling


	if (m_bDebugDraw)
	{
		if (m_bAsyncCompute)
		{
			custom::CommandQueue& graphicsQueue = device::g_commandQueueManager.GetGraphicsQueue();
			custom::CommandQueue& computeQueue  = device::g_commandQueueManager.GetComputeQueue();

			ASSERT(graphicsQueue.WaitCommandQueueCompletedGPUSide(computeQueue));
		}

		computeContext.PIXBeginEvent(L"SSAO_Debug_Draw", 0u);
		computeContext.SetResourceTransitionBarrierIndex(0u);

		computeContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0U);
		computeContext.TransitionResource(bufferManager::g_SSAOFullScreen,   D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
		computeContext.SubmitResourceBarriers(0u);

		// NewComputeContext.SetRootSignature(m_MainRootSignature);
		computeContext.SetRootSignature(m_LinearizeDepthSignature, 0u);
		computeContext.SetPipelineState(m_DebugSSAOCS, 0u);
		computeContext.SetDynamicDescriptors(1U, 0U, 1U, &bufferManager::g_SceneColorBuffer.GetUAV(), 0u);
		computeContext.SetDynamicDescriptors(2U, 0U, 1U, &bufferManager::g_SSAOFullScreen.GetSRV(), 0u);
		computeContext.Dispatch2D(bufferManager::g_SSAOFullScreen.GetWidth(), bufferManager::g_SSAOFullScreen.GetHeight(), 0u);

		computeContext.PIXEndEvent(0u); // End DebugDraw
		computeContext.PIXEndEvent(0u); // End 4_SSAOPass
		computeContext.Finish();

		custom::GraphicsContext& LastGraphicsContext = custom::GraphicsContext::GetRecentContext();
		LastGraphicsContext.SetResourceTransitionBarrierIndex(0u);
		device::g_commandContextManager.FreeContext(&LastGraphicsContext);
	}
	else
	{
		computeContext.PIXEndEvent(0u); // End m_bAsyncCompute? (L"4_Async Compute SSAO") : (L"4_Sync Compute SSAO")
		computeContext.Finish(false);
	}

	// Device Hung

#ifdef _DEBUG
	float DeltaTime2 = graphics::GetDebugFrameTime();
	m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}

void SSAOPass::ComputeAO
(
	custom::ComputeContext& Context, uint8_t commandIndex,
	ColorBuffer& Result, ColorBuffer& _DepthBuffer,
	const float TangentHalfFOVHorizontal
)
{
	size_t BufferWidth  = (size_t)_DepthBuffer.GetWidth();
	size_t BufferHeight = (size_t)_DepthBuffer.GetHeight();
	size_t ArrayCount   = (size_t)_DepthBuffer.GetDepth();

	float ThicknessMultiplier = 2.0f * TangentHalfFOVHorizontal * m_ScreenSpaceDiameter / BufferWidth;

	if (ArrayCount == 1ul)
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

		for (size_t i = 12ul; i < 24ul; ++i)
		{
			totalWeight += SsaoCB[i];
		}

		for (size_t i = 12ul; i < 24ul; ++i)
		{
			SsaoCB[i] /= totalWeight;
		}

		SsaoCB[24] = 1.0f / BufferWidth;
		SsaoCB[25] = 1.0f / BufferHeight;
		SsaoCB[26] = 1.0f / -m_RejectionFallOff;
		SsaoCB[27] = 1.0f / (1.0f + m_Accentuation);
	}

	Context.SetDynamicConstantBufferView(1U, sizeof(SsaoCB), SsaoCB, commandIndex);
	Context.SetDynamicDescriptor(2U, 0U, Result.GetUAV(), commandIndex);
	Context.SetDynamicDescriptor(3U, 0U, _DepthBuffer.GetSRV(), commandIndex);

	if (ArrayCount == 1ul)
	{
		Context.Dispatch2D(BufferWidth, BufferHeight, 16ul, 16ul, commandIndex);
	}
	else
	{
		Context.Dispatch3D(BufferWidth, BufferHeight, ArrayCount, 8ul, 8ul, 1ul, commandIndex);
	}
}

void SSAOPass::BlurAndUpsampling
(
	custom::ComputeContext& computeContext, uint8_t commandIndex,
	ColorBuffer& Result, ColorBuffer& HighResolutionDepth, ColorBuffer& LowResolutionDepth,
	ColorBuffer* InterleavedAO, ColorBuffer* HighQualityAO, ColorBuffer* HighResolutionAO
)
{
	{
		ComputePSO* computeShader = nullptr;

		if (HighResolutionAO == nullptr)
		{
			computeShader = (HighQualityAO) ? &m_BlurUpSampleFinalWithCombineLowerResolutionCS : &m_BlurUpSampleFinalWithNoneCS;
		}
		else
		{
			computeShader = (HighQualityAO) ? &m_BlurUpSampleBlendWithBothCS : &m_BlurUpSampleBlendWithHighResolutionCS;
		}

		computeContext.SetPipelineState(*computeShader, commandIndex);
	}

	size_t HighLowWidthHeight[] = 
	{ 
		LowResolutionDepth.GetWidth(), 
		LowResolutionDepth.GetHeight(), 
		HighResolutionDepth.GetWidth(), 
		HighResolutionDepth.GetHeight() 
	};

	const float kBlurTolerance = powf(1.0f - (powf(10.0f, m_BlurTolerance) * bufferManager::g_SceneColorBuffer.GetWidth() / (float)HighLowWidthHeight[0]), 2.0f);
	const float kUpsampleTolerance = powf(10.0f, m_UpsampleTolerance);
	const float kNoiseFilterWeight = 1.0f / (powf(10.0f, m_NoiseFilterTolerance) + kUpsampleTolerance);
	
	__declspec(align(16)) float cbData[] =
	{
			1.0f / HighLowWidthHeight[0], 1.0f / HighLowWidthHeight[1], 1.0f / HighLowWidthHeight[2], 1.0f / HighLowWidthHeight[3],
			kNoiseFilterWeight, 1920.0f / (float)HighLowWidthHeight[0], kBlurTolerance, kUpsampleTolerance
	};

	computeContext.SetDynamicConstantBufferView(1, sizeof(cbData), &cbData, commandIndex);

	computeContext.TransitionResource(Result,              D3D12_RESOURCE_STATE_UNORDERED_ACCESS,          0U);
	computeContext.TransitionResource(LowResolutionDepth,  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
	computeContext.TransitionResource(HighResolutionDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
	computeContext.SubmitResourceBarriers(commandIndex); // TODO 0 : ASSERT(false); 멀티스레딩하게되면 여기가 중점
	computeContext.SetDynamicDescriptor(2U, 0U, Result.GetUAV(),              commandIndex);
	computeContext.SetDynamicDescriptor(3U, 0U, LowResolutionDepth.GetSRV(),  commandIndex);
	computeContext.SetDynamicDescriptor(3U, 1U, HighResolutionDepth.GetSRV(), commandIndex);

	if (InterleavedAO != nullptr)
	{
		computeContext.TransitionResource(*InterleavedAO, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
		computeContext.SubmitResourceBarriers(commandIndex);
		computeContext.SetDynamicDescriptor(3U, 2U, InterleavedAO->GetSRV(), commandIndex);
	}
	if (HighQualityAO != nullptr)
	{
		computeContext.TransitionResource(*HighQualityAO, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
		computeContext.SubmitResourceBarriers(commandIndex);
		computeContext.SetDynamicDescriptor(3U, 3U, HighQualityAO->GetSRV(), commandIndex);
	}
	if (HighResolutionAO != nullptr)
	{
		computeContext.TransitionResource(*HighResolutionAO, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
		computeContext.SubmitResourceBarriers(commandIndex);
		computeContext.SetDynamicDescriptor(3U, 4U, HighResolutionAO->GetSRV(), commandIndex);
	}

	computeContext.Dispatch2D(HighLowWidthHeight[2] + 2, HighLowWidthHeight[3] + 2, 16, 16, commandIndex);
}