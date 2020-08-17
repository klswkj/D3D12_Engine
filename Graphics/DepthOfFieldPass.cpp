#include "stdafx.h"
#include "DepthOfFieldPass.h"

#include "RawBuffer.h"
#include "GPUResource.h"
#include "UAVBuffer.h"
#include "CommandContext.h"
#include "GraphicsContext.h"
#include "ComputeContext.h"
#include "BufferManager.h"
#include "PreMadePSO.h"

// TODO : Set Branch and Shader Code.

DOFPass::DOFPass(const char* name)
	: Pass(name)
{
	m_RootSignature.Reset(4, 3);
	m_RootSignature.InitStaticSampler(0, premade::g_SamplerPointBorderDesc);
	m_RootSignature.InitStaticSampler(1, premade::g_SamplerPointClampDesc);
	m_RootSignature.InitStaticSampler(2, premade::g_SamplerLinearClampDesc);
	m_RootSignature[0].InitAsConstantBuffer(0);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6);
	m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3);
	m_RootSignature[3].InitAsConstants(1, 1);
	m_RootSignature.Finalize(L"Depth of Field");

	// -> CreatPSO Part
#define CREATE_PSO( PSOName, ShaderByteCode) \
	PSOName.SetRootSignature(s_RootSignature); \
	PSOName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode));\
	PSOName.Finalize();

	// If compiled Shader Code was added later, Fill this part. 


#undef CREATE_PSO

	if (device::g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
	{
		uint32_t BufferWidth = bufferManager::g_SceneColorBuffer.GetWidth();
		uint32_t BufferHeight = bufferManager::g_SceneColorBuffer.GetHeight();
		uint32_t TiledWidth = bufferManager::g_DoFTileClass[0].GetWidth(); // DXGI_FORMAT_R11G11B10_FLOAT
		uint32_t TiledHeight = bufferManager::g_DoFTileClass[0].GetHeight();

		cBuffer =
		{
			(float)m_FocalDepth, 1.0f / (float)m_FocalRange,
			(float)m_FocalDepth - (float)m_FocalRange, (float)m_FocalDepth + (float)m_FocalRange,
			1.0f / BufferWidth, 1.0f / BufferHeight,
			BufferWidth, BufferHeight,
			(int32_t)Math::DivideByMultiple(BufferWidth, 2), (int32_t)Math::DivideByMultiple(BufferHeight, 2),
			TiledWidth, TiledHeight,
			1.0f / TiledWidth, 1.0f / TiledHeight,
			(uint32_t)m_bDebugMode, m_bEnablePreFilter ? 1u : 0u, // TODO :  Must Check PreFilter on Shader Code.
			0.01f, 100.0f, (float)m_AntiSparkleWeight
		};
	}

    __declspec(align(16)) const uint32_t initArgs[9] = { 0, 1, 1, 0, 1, 1, 0, 1, 1 };
    m_IndirectArgument.Create(L"DoF Indirect Parameters", 3, sizeof(D3D12_DISPATCH_ARGUMENTS), initArgs);

	RawLayout Coeff;

	Coeff.Add<custom::Type::Bool>("Enable");

	// bEnable = std::make_shared<CachingPixelConstantBufferEx>
	// bEnable;
}

DOFPass::~DOFPass()
{
    m_IndirectArgument.Destroy();
}

void DOFPass::Execute(custom::CommandContext& BaseContext, float NearClipDist, float FarClipDist) DEBUG_EXCEPT
{
    custom::CommandContext& BaseContext = custom::CommandContext::Begin(L"DOFPass Execute");
    custom::ComputeContext& computeContext = BaseContext.GetComputeContext();

	if (!device::g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
	{
		return;
	}

	ColorBuffer& LinearDepth = bufferManager::g_LinearDepth[device::GetFrameCount() % device::g_DisplayBufferCount]; // DXGI_FORMAT_R16_UNORM

	uint32_t BufferWidth  = LinearDepth.GetWidth();
	uint32_t BufferHeight = LinearDepth.GetHeight();
	uint32_t TiledWidth   = bufferManager::g_DoFTileClass[0].GetWidth(); // DXGI_FORMAT_R11G11B10_FLOAT
	uint32_t TiledHeight  = bufferManager::g_DoFTileClass[0].GetHeight();

	cBuffer.FGRange = m_ForegroundRange / FarClipDist;
	cBuffer.RcpFGRange = 1.0f / cBuffer.FGRange;

	using namespace custom;
	{
		// Initial pass to discover max CoC and closest depth in 16x16 tiles
		computeContext.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DoFTileClass[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.SetPipelineState(DoFPass1CS);
		computeContext.SetDynamicDescriptor(1, 0, LinearDepth.GetSRV());
		computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_DoFTileClass[0].GetUAV());
		computeContext.Dispatch2D(BufferWidth, BufferHeight, 16, 16);

		computeContext.ResetCounter(bufferManager::g_DoFWorkQueue);
		computeContext.ResetCounter(bufferManager::g_DoFFastQueue);
		computeContext.ResetCounter(bufferManager::g_DoFFixupQueue);

		// 3x3 filter to spread max CoC and closest depth to neighboring tiles
		computeContext.TransitionResource(bufferManager::g_DoFTileClass[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DoFTileClass[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_DoFWorkQueue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_DoFFastQueue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.SetPipelineState(DoFTilePassCS);
		computeContext.SetDynamicDescriptor(1, 0, bufferManager::g_DoFTileClass[0].GetSRV());
		computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_DoFTileClass[1].GetUAV());
		computeContext.SetDynamicDescriptor(2, 1, bufferManager::g_DoFWorkQueue.GetUAV());
		computeContext.SetDynamicDescriptor(2, 2, bufferManager::g_DoFFastQueue.GetUAV());
		computeContext.Dispatch2D(TiledWidth, TiledHeight);

		computeContext.TransitionResource(bufferManager::g_DoFTileClass[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DoFFixupQueue, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.SetPipelineState(DoFTilePassFixupCS);
		computeContext.SetDynamicDescriptor(1, 0, bufferManager::g_DoFTileClass[1].GetSRV());
		computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_DoFFixupQueue.GetUAV());
		computeContext.Dispatch2D(TiledWidth, TiledHeight);

		computeContext.TransitionResource(bufferManager::g_DoFWorkQueue, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.CopyCounter(m_IndirectArgument, 0, bufferManager::g_DoFWorkQueue);

		computeContext.TransitionResource(bufferManager::g_DoFFastQueue, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.CopyCounter(m_IndirectArgument, 12, bufferManager::g_DoFFastQueue);

		computeContext.TransitionResource(bufferManager::g_DoFFixupQueue, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.CopyCounter(m_IndirectArgument, 24, bufferManager::g_DoFFixupQueue);

		computeContext.TransitionResource(m_IndirectArgument, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	}

	{
		// if (ForceFast && !DebugMode)
		// Context.SetPipelineState(s_DoFPreFilterFastCS);
	    // else
		// Context.SetPipelineState(s_DoFPreFilterCS);
#ifdef _DEBUG
		computeContext.SetPipelineState(DoFPreFilterFastCS);
#else
        computeContext.SetPipelineState(DoFPreFilterCS);
#endif
		computeContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DoFPresortBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_DoFPrefilter, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.SetDynamicDescriptor(1, 0, LinearDepth.GetSRV());
		computeContext.SetDynamicDescriptor(1, 1, bufferManager::g_DoFTileClass[1].GetSRV());
		computeContext.SetDynamicDescriptor(1, 2, bufferManager::g_SceneColorBuffer.GetSRV());
		computeContext.SetDynamicDescriptor(1, 3, bufferManager::g_DoFWorkQueue.GetSRV());
		computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_DoFPresortBuffer.GetUAV());
		computeContext.SetDynamicDescriptor(2, 1, bufferManager::g_DoFPrefilter.GetUAV());
		computeContext.DispatchIndirect(m_IndirectArgument, 0);

		// if (!m_DebugMode)
		// Context.SetPipelineState(s_DoFPreFilterFastCS);
#ifdef _DEBUG
		computeContext.SetPipelineState(DoFPreFilterFastCS);
#endif
		computeContext.SetDynamicDescriptor(1, 3, bufferManager::g_DoFFastQueue.GetSRV());
		computeContext.DispatchIndirect(m_IndirectArgument, 12);

		computeContext.SetPipelineState(DoFPreFilterFixupCS);
		computeContext.SetDynamicDescriptor(1, 3, bufferManager::g_DoFFixupQueue.GetSRV());
		computeContext.DispatchIndirect(m_IndirectArgument, 24);
	}

	{
		// DoF Main Pass
		computeContext.PIXBeginEvent(L"DOF Main Pass");

		computeContext.TransitionResource(bufferManager::g_DoFPrefilter,    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		computeContext.TransitionResource(bufferManager::g_DoFBlurColor[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		computeContext.TransitionResource(bufferManager::g_DoFBlurAlpha[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//if (!m_DebugMode)
		//	Context.SetPipelineState(s_DoFPass2FastCS);
		//else
		//	Context.SetPipelineState(s_DoFPass2DebugCS); -> Debug Mode Computer-Shader is Deleted.
#ifdef _DEBUG
		computeContext.SetPipelineState(DoFPass2FastCS);
#else
		computeContext.SetPipelineState(DoFPass2CS); // DebugMode > 0 ? s_DoFPass2DebugCS : s_DoFPass2CS
#endif // (_DEBUG)
		computeContext.SetDynamicDescriptor(1, 0, bufferManager::g_DoFPrefilter.GetSRV());
		computeContext.SetDynamicDescriptor(1, 1, bufferManager::g_DoFPresortBuffer.GetSRV());
		computeContext.SetDynamicDescriptor(1, 2, bufferManager::g_DoFTileClass[1].GetSRV());
		computeContext.SetDynamicDescriptor(1, 3, bufferManager::g_DoFWorkQueue.GetSRV());
		computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_DoFBlurColor[0].GetUAV());
		computeContext.SetDynamicDescriptor(2, 1, bufferManager::g_DoFBlurAlpha[0].GetUAV());
		computeContext.DispatchIndirect(m_IndirectArgument, 0);
		
		//if (!ForceSlow && !DebugMode)
		//{
		//	Context.SetPipelineState(s_DoFPass2FastCS);
		//}
#ifdef _DEBUG
		computeContext.SetPipelineState(DoFPass2FastCS);
#endif // (_DEBUG)

		computeContext.SetDynamicDescriptor(1, 3, bufferManager::g_DoFFastQueue.GetSRV());
		computeContext.DispatchIndirect(m_IndirectArgument, 12);

		computeContext.SetPipelineState(DoFPass2FixupCS);
		computeContext.SetDynamicDescriptor(1, 3, bufferManager::g_DoFFixupQueue.GetSRV());
		computeContext.DispatchIndirect(m_IndirectArgument, 24);

		computeContext.PIXEndEvent();
	}

	{
		computeContext.PIXBeginEvent(L"DOF Median Pass");

		computeContext.TransitionResource(bufferManager::g_DoFBlurColor[0], D3D12_RESOURCE_STATE_GENERIC_READ);
		computeContext.TransitionResource(bufferManager::g_DoFBlurAlpha[0], D3D12_RESOURCE_STATE_GENERIC_READ);

		// if (m_bMedianFilter)
		{
			computeContext.PIXBeginEvent(L"Median Filter");

			computeContext.TransitionResource(bufferManager::g_DoFBlurColor[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext.TransitionResource(bufferManager::g_DoFBlurAlpha[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			computeContext.SetPipelineState(DoFMedianFilterCS); // MedianAlpha ? s_DoFMedianFilterSepAlphaCS : s_DoFMedianFilterCS
			computeContext.SetDynamicDescriptor(1, 0, bufferManager::g_DoFBlurColor[0].GetSRV());
			computeContext.SetDynamicDescriptor(1, 1, bufferManager::g_DoFBlurAlpha[0].GetSRV());
			computeContext.SetDynamicDescriptor(1, 2, bufferManager::g_DoFWorkQueue.GetSRV());
			computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_DoFBlurColor[1].GetUAV());
			computeContext.SetDynamicDescriptor(2, 1, bufferManager::g_DoFBlurAlpha[1].GetUAV());
			computeContext.DispatchIndirect(m_IndirectArgument, 0);

			computeContext.SetDynamicDescriptor(1, 2, bufferManager::g_DoFFastQueue.GetSRV());
			computeContext.DispatchIndirect(m_IndirectArgument, 12);

			computeContext.SetPipelineState(DoFMedianFilterFixupCS);
			computeContext.SetDynamicDescriptor(1, 2, bufferManager::g_DoFFixupQueue.GetSRV());
			computeContext.DispatchIndirect(m_IndirectArgument, 24);

			computeContext.TransitionResource(bufferManager::g_DoFBlurColor[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			computeContext.TransitionResource(bufferManager::g_DoFBlurAlpha[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			computeContext.PIXEndEvent();
		}
		computeContext.PIXEndEvent();
	}

	// // ScopedTimer _prof2(L"DoF Final Combine", Context);

	// if (m_bDebugTile)
	//{
	//	Context.SetPipelineState(s_DoFDebugRedCS);
	//	Context.SetDynamicDescriptor(1, 5, g_DoFWorkQueue.GetSRV());
	//	Context.SetDynamicDescriptor(2, 0, g_SceneColorBuffer.GetUAV());
	//	Context.DispatchIndirect(s_IndirectParameters, 0);
	//
	//	Context.SetPipelineState(s_DoFDebugGreenCS);
	//	Context.SetDynamicDescriptor(1, 5, g_DoFFastQueue.GetSRV());
	//	Context.DispatchIndirect(s_IndirectParameters, 12);
	//
	//	Context.SetPipelineState(s_DoFDebugBlueCS);
	//	Context.SetDynamicDescriptor(1, 5, g_DoFFixupQueue.GetSRV());
	//	Context.DispatchIndirect(s_IndirectParameters, 24);
	//}
	// else
	{
		computeContext.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);/**/

		computeContext.SetPipelineState(DoFCombineCS);
		computeContext.SetDynamicDescriptor(1, 0, bufferManager::g_DoFBlurColor[1].GetSRV()); // MedianFilter ? 1 : 0
		computeContext.SetDynamicDescriptor(1, 1, bufferManager::g_DoFBlurAlpha[1].GetSRV()); // MedianFilter ? 1 : 0
		computeContext.SetDynamicDescriptor(1, 2, bufferManager::g_DoFTileClass[1].GetSRV()); // MedianFilter ? 1 : 0
		computeContext.SetDynamicDescriptor(1, 3, LinearDepth.GetSRV());
		computeContext.SetDynamicDescriptor(1, 4, bufferManager::g_DoFWorkQueue.GetSRV());
		computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_SceneColorBuffer.GetUAV());
		computeContext.DispatchIndirect(m_IndirectArgument, 0);

		computeContext.SetPipelineState(DoFCombineFastCS);
		computeContext.SetDynamicDescriptor(1, 4, bufferManager::g_DoFFastQueue.GetSRV());
		computeContext.DispatchIndirect(m_IndirectArgument, 12);
		
		computeContext.InsertUAVBarrier(bufferManager::g_SceneColorBuffer);
	}
    // computeContext.Finish();
    // graphicsContext.Finish();
}

void DOFPass::RenderSubWindow()
{
	bool bChangedFocalDepth{ false };
	bool bChangedFocalRange{ false };
	bool bChangedForegroundRange{ false };
	bool bChangedAntiSparkleWeight{ false };
	static const auto dcheck = [](bool d, bool& carry) { carry = carry || d; };

	if (!device::g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
	{
		ImGui::Text("DOF(R11G11B10_FLOAT) not supported.");
		return;
	}

	ImGui::Checkbox("Depth of Field", (bool*)&m_bEnable);

	if (m_bEnable)
	{
		dcheck(ImGui::SliderFloat("Focal Depth", &m_FocalDepth, m_FocalRange + 0.01f, 1.0f, "%.01f"), bChangedFocalDepth);
		dcheck(ImGui::SliderFloat("Focal Range", &m_FocalRange, 0.01f, 1.0f, "%.01f"), bChangedFocalRange);
		dcheck(ImGui::SliderFloat("Foreground", &m_ForegroundRange, 10.0f, 1000.0f, "10.0f"), bChangedForegroundRange);
		dcheck(ImGui::SliderFloat("AntiSparkleWeight", &cBuffer.AntiSparkleFilterStrength, 10.0f, 1000.0f, "10.0f"), bChangedAntiSparkleWeight);
		if(ImGui::Checkbox("PreFilter", (bool*)&m_bEnablePreFilter))    // On Shader Code
		{
			cBuffer.PreFilterState = m_bEnablePreFilter;
		}
		ImGui::Checkbox("Median Filter", (bool*)&m_bMedianFilter); // On Cpu
		ImGui::Checkbox("Median Alpha", (bool*)&m_bMedianAlpha); // On Cpu
		if(ImGui::Checkbox("Debug Mode", (bool*)&m_bDebugMode)) // On Shader Code
		{
			cBuffer.DebugState = m_bDebugMode;
		}
		ImGui::Checkbox("Debug Tile", (bool*)&m_bDebugTile); // On Cpu
	}

	if (bChangedFocalDepth)
	{
		cBuffer.FocalCenter = m_FocalDepth;
	}
	if (bChangedFocalRange)
	{
		cBuffer.FocalSpread = 1.0f / (float)m_FocalRange;
	}
	if (bChangedFocalDepth || bChangedFocalRange)
	{
		cBuffer.FocalMinZ = m_FocalDepth - m_FocalRange;
		cBuffer.FocalMaxZ = m_FocalDepth + m_FocalRange;
	}
}