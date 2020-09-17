#pragma once
#include "stdafx.h"
#include "PixelBuffer.h"

class DepthBuffer : public PixelBuffer
{
public:
    friend class RenderTarget;
    friend class DepthStencil;

    DepthBuffer(float ClearDepth = 0.0f, uint8_t ClearStencil = 0)
        : m_ClearDepth(ClearDepth), m_ClearStencil(ClearStencil), m_Format(DXGI_FORMAT_UNKNOWN)
    {
        m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format);

    void CreateSamples(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format);

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
    void createDSV(ID3D12Device* Device, DXGI_FORMAT Format);

protected:
    float m_ClearDepth;
    uint8_t m_ClearStencil;
    DXGI_FORMAT m_Format;

    D3D12_CPU_DESCRIPTOR_HANDLE m_hDSV[4]; // -> DSV(0), DSV_DepthReadOnly(1), DSV_StencilReadOnly(2),  DSV_ReadOnly(3)
    D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;
};