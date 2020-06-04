#pragma once
#include "stdafx.h"
#include "DescriptorHeapManager.h"

namespace graphics
{
    extern DescriptorHeapManager g_DescriptorHeapManager;

    inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
    {
        return g_DescriptorHeapManager.Allocate(Type, Count);
    }
}