#pragma once
#include "stdafx.h"
#include "PreMadePSO.h"
#include "DescriptorHeapManager.h"

namespace custom
{
    class RootSignature;
    class CommandSignature;
}

namespace graphics
{
    void Initialize();
    void Resize(uint32_t Width, uint32_t Height);
    void Present();
    void Terminate();
    void ShutDown();

    uint64_t GetFrameCount();
    float GetFrameTime();
    float GetFrameRate();

    // Device
    /*
    extern ID3D12Device* g_Device;
    extern CommandListManager g_CommandManager;
    extern ContextManager g_ContextManager;

    extern D3D_FEATURE_LEVEL g_D3DFeatureLevel;
    extern bool g_bTypedUAVLoadSupport_R11G11B10_FLOAT;
    extern bool g_bEnableHDROutput;

    extern DescriptorAllocator g_DescriptorAllocator[];
    inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor( D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1 )
    {
        return g_DescriptorAllocator[Type].Allocate(Count);
    }
    */

    extern custom::RootSignature g_GenerateMipsRS;
    extern ComputePSO            g_GenerateMipsLinearPSO[4];
    extern ComputePSO            g_GenerateMipsGammaPSO[4];

    extern DescriptorHeapManager g_DescriptorHeapManager;

    inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
    {
        return g_DescriptorHeapManager.Allocate(Type, Count);
    }

    extern custom::CommandSignature g_DispatchIndirectCommandSignature;
    extern custom::CommandSignature g_DrawIndirectCommandSignature;
}