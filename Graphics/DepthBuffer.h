#pragma once
#include "stdafx.h"
#include "PixelBuffer.h"

#ifndef D3D12_RESOURCE_BARRIER_DEPTH_SUBRESOURCE
#define D3D12_RESOURCE_BARRIER_DEPTH_SUBRESOURCE 0U
#endif

#ifndef D3D12_RESOURCE_BARRIER_STENCIIL_SUBRESOURCE
#define D3D12_RESOURCE_BARRIER_STENCIL_SUBRESOURCE 1U
#endif

#ifndef D3D12_RESOURCE_BARRIER_DEPTH_SUBRESOURCE_BITFLAG
#define D3D12_RESOURCE_BARRIER_DEPTH_SUBRESOURCE_BITFLAG 1U
#endif

#ifndef D3D12_RESOURCE_BARRIER_STENCIL_SUBRESOURCE_BITFLAG
#define D3D12_RESOURCE_BARRIER_STENCIL_SUBRESOURCE_BITFLAG 2U
#endif

#ifndef D3D12_RESOURCE_BARRIER_DEPTH_STENCIL_SUBRESOURCE_BITFLAG
#define D3D12_RESOURCE_BARRIER_DEPTH_STENCIL_SUBRESOURCE_BITFLAG 3U
#endif

class DepthBuffer : public PixelBuffer
{
public:
    friend class RenderTarget;
    friend class DepthStencil;

    DepthBuffer(const float ClearDepth = 0.0f, const uint8_t ClearStencil = 0u)
        : 
        m_ClearDepth(ClearDepth), 
        m_ClearStencil(ClearStencil), 
        m_Format(DXGI_FORMAT_UNKNOWN)
    {
        m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }
    virtual ~DepthBuffer() = default;

    void Create(const std::wstring& wName, const uint32_t width, const uint32_t height, const DXGI_FORMAT format);

    void CreateSamples(const std::wstring& Name, const uint32_t width, const uint32_t height, const uint32_t numSamples, const DXGI_FORMAT format);

    // Get pre-created CPU-visible descriptor handles
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() const 
    { 
        return m_hDSV[0]; 
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_DepthReadOnly() const 
    { 
        return m_hDSV[1]; 
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_StencilReadOnly() const 
    { 
        return m_hDSV[2]; 
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_ReadOnly() const 
    { 
        return m_hDSV[3]; 
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDepthSRV() const 
    { 
        return m_hDepthSRV; 
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetStencilSRV() const 
    { 
        return m_hStencilSRV; 
    }

    float GetClearDepth() const 
    { 
        return m_ClearDepth; 
    }
    uint8_t GetClearStencil() const 
    { 
        return m_ClearStencil; 
    }
    DXGI_FORMAT GetFormat() const
    {
        return m_Format;
    }

private:
    void createDSV(ID3D12Device* const pDevice, const DXGI_FORMAT format);

protected:
    const float   m_ClearDepth;
    const uint8_t m_ClearStencil;
    DXGI_FORMAT   m_Format;

    D3D12_CPU_DESCRIPTOR_HANDLE m_hDSV[4]; // -> DSV(0), DSV_DepthReadOnly(1), DSV_StencilReadOnly(2),  DSV_ReadOnly(3)
    D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;
};