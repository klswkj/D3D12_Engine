#pragma once
#include "Color.h"
#include "PixelBuffer.h"

namespace custom
{
    class CommandContext;
    class ComputeContext;
}

class ColorBuffer : public PixelBuffer
{
public:
    ColorBuffer(custom::Color ClearColor = custom::Color(0.0f, 0.0f, 0.0f, 0.0f))
        : 
        m_clearColor(ClearColor), 
        m_numMipMaps(0), 
        m_fragmentCount(1), 
        m_sampleCount(1)
    {
        m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        std::memset(m_UAVHandle, 0xFF, sizeof(m_UAVHandle));
    }

    // Create a color buffer from a swap chain buffer.  Unordered access is restricted.
    void CreateFromSwapChain(const std::wstring& Name, ID3D12Resource* BaseResource);

    void CreateCommitted
    (
        const std::wstring& Name, 
        const uint32_t width, const uint32_t height, uint32_t numMips,
        const DXGI_FORMAT format, const bool bRenderTarget = false
    );

    void CreateCommittedArray
    (
        const std::wstring& Name, const uint32_t width, const uint32_t height,
        const uint32_t arrayCount, const DXGI_FORMAT Format, const bool bRenderTarget = false
    );

    // Get pre-created CPU-visible descriptor handles
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const 
    {
        return m_SRVHandle; 
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV() const 
    {
        return m_RTVHandle; // m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const 
    { 
        return m_UAVHandle[0]; 
    }

    void SetClearColor(const custom::Color& clearColor) 
    { 
        m_clearColor = clearColor; 
    }

    void SetMsaaMode(const uint32_t numColorSamples, const uint32_t numCoverageSamples)
    {
        ASSERT(numCoverageSamples >= numColorSamples);
        m_fragmentCount = numColorSamples;
        m_sampleCount = numCoverageSamples;
    }

    custom::Color GetClearColor() const 
    { 
        return m_clearColor; 
    }

    void GenerateMipMaps(custom::ComputeContext& computeContext, const uint8_t commandIndex);

protected:
    D3D12_RESOURCE_FLAGS CombineResourceFlags(const bool bRenderTarget) const
    {
        D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

		if (Flags == D3D12_RESOURCE_FLAG_NONE && m_fragmentCount == 1)
		{
			Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

        return (D3D12_RESOURCE_FLAGS)bRenderTarget | Flags;
    }

    // Compute the number of texture levels needed to reduce to 1x1.  This uses
    // _BitScanReverse to find the highest set bit.  Each dimension reduces by
    // half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
    // as the dimension 511 (0x1FF).
    static inline uint32_t ComputeNumMips(const uint32_t width, const uint32_t height)
    {
        uint32_t HighBit;
        _BitScanReverse((unsigned long*)&HighBit, width | height);
        return HighBit + 1;
    }
    void createResourceViews
    (
        ID3D12Device* const Device, 
        const DXGI_FORMAT format, 
        const uint32_t arraySize, 
        const uint32_t numMips = 1,
        const bool bRenderTarget = false
    );

protected:
    custom::Color m_clearColor; 
    uint32_t m_numMipMaps; // number of texture sublevels
    uint32_t m_fragmentCount;
    uint32_t m_sampleCount;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
    
};
