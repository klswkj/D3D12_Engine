#pragma once
#include "stdafx.h"

namespace custom
{
    class GPUResource;
}

class PixelBuffer : public custom::GPUResource
{
public:
    PixelBuffer() 
        : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN), m_BankRotation(0) 
    {
    }

    uint32_t GetWidth(void) const 
    { 
        return m_Width; 
    }
    uint32_t GetHeight(void) const 
    { 
        return m_Height; 
    }
    uint32_t GetDepth(void) const 
    { 
        return m_ArraySize; 
    }
    const DXGI_FORMAT& GetFormat(void) const 
    { 
        return m_Format; 
    }

    // Has no effect on Windows
    void SetBankRotation(uint32_t RotationAmount) 
    { 
        m_BankRotation = RotationAmount; 
    }

protected:

    D3D12_RESOURCE_DESC Texture2DResourceDescriptor(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);

    void CopyResource(ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState);

    void CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
        D3D12_CLEAR_VALUE ClearValue);

    static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
    static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
    static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
    static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
    static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
    static size_t BytesPerPixel(DXGI_FORMAT Format);

    uint32_t    m_Width;
    uint32_t    m_Height;
    uint32_t    m_ArraySize;
    uint32_t    m_BankRotation;
    DXGI_FORMAT m_Format;
};
