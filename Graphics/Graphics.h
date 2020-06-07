#pragma once
#include "stdafx.h"
#include "ReadyMadePSO.h"
#include "DescriptorHeapManager.h"

namespace custom
{
    class CommandSignature;
}

namespace graphics
{
    void Initialize();

    extern custom::RootSignature g_GenerateMipsRS;
    extern ComputePSO            g_GenerateMipsLinearPSO[4];
    extern ComputePSO            g_GenerateMipsGammaPSO[4];

    extern DescriptorHeapManager g_DescriptorHeapManager;

    inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
    {
        return g_DescriptorHeapManager.Allocate(Type, Count);
    }

    extern custom::CommandSignature DispatchIndirectCommandSignature;
    extern custom::CommandSignature DrawIndirectCommandSignature;
}