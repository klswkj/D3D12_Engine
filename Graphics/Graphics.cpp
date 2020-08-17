#include "stdafx.h"
#include "Graphics.h"
#include "RootSignature.h"
#include "CommandSignature.h"

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

    ComputePSO  g_GenerateMipsLinearPSO[4];
    ComputePSO  g_GenerateMipsGammaPSO[4];

    custom::RootSignature s_PresentRS;

    custom::RootSignature g_GenerateMipsRS;

	DescriptorHeapManager g_DescriptorHeapManager;

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

void graphics::Initialize()
{

}

