#pragma once
#include "stdafx.h"
#include "GPUResource.h"

class PixelBuffer : public custom::GPUResource
{
public:
    PixelBuffer() 
        : 
        m_width(0u), m_height(0u), m_arraySize(0u), m_format(DXGI_FORMAT_UNKNOWN)
    {
    }

    virtual ~PixelBuffer() = default;

    uint32_t GetWidth() const 
    { 
        return m_width; 
    }
    uint32_t GetHeight() const 
    { 
        return m_height; 
    }
    uint32_t GetDepth() const 
    { 
        return m_arraySize; 
    }
    const DXGI_FORMAT& GetFormat() const 
    { 
        return m_format; 
    }

protected:
    D3D12_RESOURCE_DESC Texture2DResourceDescriptor
    (
        const uint32_t width, const uint32_t height, const uint32_t depthOrArraySize,
        const uint32_t numMips, const DXGI_FORMAT format, const UINT flags
    );

    void CopyResource
    (
        ID3D12Device* const pDevice, const std::wstring& wName,
        ID3D12Resource* const pResource, const D3D12_RESOURCE_STATES* const pCurrentResourceStates, const D3D12_RESOURCE_STATES* const pPendingResourceStates = nullptr,
        const uint8_t numSubResource = 1u
    );

    void CreateTextureCommittedResource
    (
        ID3D12Device* const pDevice, const std::wstring& wName,
        const D3D12_RESOURCE_DESC& resourceDesc,
        const D3D12_CLEAR_VALUE* const pClearValue
    );

    static DXGI_FORMAT GetBaseFormat   (const DXGI_FORMAT defaultFormat);
    static DXGI_FORMAT GetUAVFormat    (const DXGI_FORMAT defaultFormat);
    static DXGI_FORMAT GetDSVFormat    (const DXGI_FORMAT defaultFormat);
    static DXGI_FORMAT GetDepthFormat  (const DXGI_FORMAT defaultFormat);
    static DXGI_FORMAT GetStencilFormat(const DXGI_FORMAT defaultFormat);
    static size_t      BytesPerPixel   (const DXGI_FORMAT defaultFormat);

protected:
    uint32_t    m_width;
    uint32_t    m_height;
    DXGI_FORMAT m_format;
    uint32_t    m_arraySize;
};
