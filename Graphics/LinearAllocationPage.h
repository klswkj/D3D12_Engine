#pragma once
#include "stdafx.h"
#include "GPUResource.h"

class LinearAllocationPage : public custom::GPUResource
{
public:
    LinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Usage) : custom::GPUResource()
    {
        SafeRelease(pResource);
        m_pResource = pResource;
        //m_pResource.Attach(pResource);
        m_currentState = Usage;
        m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
        m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
    }

    ~LinearAllocationPage()
    {
        SafeUnmap();
    }

    void SafeMap(void)
    {
        if (m_CpuVirtualAddress == nullptr)
        {
            m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
        }
    }

    void SafeUnmap(void)
    {
        if (m_CpuVirtualAddress != nullptr)
        {
            m_pResource->Unmap(0, nullptr);
            m_CpuVirtualAddress = nullptr;
        }
    }

    void* m_CpuVirtualAddress;
    D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
};