#include "stdafx.h"
#include "Graphics.h"
#include "PreMadePSO.h"
#include "RootSignature.h"
#include "CommandSignature.h"

#include "TextureManager.h"

namespace graphics
{
    GraphicsPSO s_BlendUIPSO;
    GraphicsPSO PresentSDRPS;
    GraphicsPSO PresentHDRPS;
    GraphicsPSO MagnifyPixelsPS;
    GraphicsPSO SharpeningUpsamplePS;
    GraphicsPSO BicubicHorizontalUpsamplePS;
    GraphicsPSO BicubicVerticalUpsamplePS;
    GraphicsPSO BilinearUpsamplePS;

    custom::RootSignature g_GenerateMipsRootSignature;
    custom::RootSignature g_PresentRootSignature;
	DescriptorHeapManager g_DescriptorHeapManager;

    ComputePSO  g_GenerateMipsLinearPSO[4];
    ComputePSO  g_GenerateMipsGammaPSO[4];

    custom::CommandSignature g_DispatchIndirectCommandSignature(1); // goto ReadyMadePSO
    custom::CommandSignature g_DrawIndirectCommandSignature(1);     // goto ReadyMadePSO

    // CommandSignature는 ComputeContext::DispatchIndirect에서 계속 재활용
}

namespace
{
    float s_FrameTime = 0.0f;
    uint64_t s_FrameIndex = 0;
    int64_t s_FrameStartTick = 0;
}

void graphics::ShutDown()
{
    TextureManager::Shutdown();
}

void graphics::Initialize()
{
    g_GenerateMipsRootSignature.Reset(3, 1);
    g_GenerateMipsRootSignature[0].InitAsConstants(0, 4);
    g_GenerateMipsRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_GenerateMipsRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4);
    g_GenerateMipsRootSignature.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
    g_GenerateMipsRootSignature.Finalize(L"Generate Mips");

    g_PresentRootSignature.Reset(4, 2);
    g_PresentRootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    g_PresentRootSignature[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
    g_PresentRootSignature[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    g_PresentRootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    g_PresentRootSignature.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
    g_PresentRootSignature.InitStaticSampler(1, premade::g_SamplerPointClampDesc);
    g_PresentRootSignature.Finalize(L"Present");


}

