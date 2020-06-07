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

    custom::CommandSignature DispatchIndirectCommandSignature(1);
    custom::CommandSignature DrawIndirectCommandSignature(1);
}