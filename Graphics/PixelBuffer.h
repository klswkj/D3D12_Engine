#pragma once
#include "stdafx.h"
#include "GPUResource.h"

class PixelBuffer : public custom::GPUResource
{
public:
    PixelBuffer() 
        : m_width(0), m_height(0), m_arraySize(0), m_format(DXGI_FORMAT_UNKNOWN)
    {
    }

    uint32_t GetWidth(void) const 
    { 
        return m_width; 
    }
    uint32_t GetHeight(void) const 
    { 
        return m_height; 
    }
    uint32_t GetDepth(void) const 
    { 
        return m_arraySize; 
    }
    const DXGI_FORMAT& GetFormat(void) const 
    { 
        return m_format; 
    }

protected:
    D3D12_RESOURCE_DESC Texture2DResourceDescriptor
    (
        uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, 
        uint32_t NumMips, DXGI_FORMAT Format, UINT Flags
    );

    void CopyResource(ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState);

    void CreateTextureResource
    (
        ID3D12Device* Device, const std::wstring& Name, 
        const D3D12_RESOURCE_DESC& ResourceDesc,
        D3D12_CLEAR_VALUE ClearValue
    );

    static DXGI_FORMAT GetBaseFormat   (DXGI_FORMAT Format);
    static DXGI_FORMAT GetUAVFormat    (DXGI_FORMAT Format);
    static DXGI_FORMAT GetDSVFormat    (DXGI_FORMAT Format);
    static DXGI_FORMAT GetDepthFormat  (DXGI_FORMAT Format);
    static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
    static size_t      BytesPerPixel   (DXGI_FORMAT Format);

protected:
    uint32_t    m_width;
    uint32_t    m_height;
    uint32_t    m_arraySize;
    DXGI_FORMAT m_format;
};
