#pragma once
#include "Color.h"
#include "PixelBuffer.h"

namespace custom
{
    class CommandContext;
}

class ColorBuffer : public PixelBuffer
{
public:
    ColorBuffer(custom::Color ClearColor = custom::Color(0.0f, 0.0f, 0.0f, 0.0f))
        : m_clearColor(ClearColor), m_numMipMaps(0), m_fragmentCount(1), m_sampleCount(1)
    {
        m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        std::memset(m_UAVHandle, 0xFF, sizeof(m_UAVHandle));
    }

    // Create a color buffer from a swap chain buffer.  Unordered access is restricted.
    void CreateFromSwapChain(const std::wstring& Name, ID3D12Resource* BaseResource);

    void Create
    (
        const std::wstring& Name, 
        uint32_t Width, uint32_t Height, uint32_t NumMips,
        DXGI_FORMAT Format
    );

    void CreateArray
    (
        const std::wstring& Name, uint32_t Width, uint32_t Height,
        uint32_t ArrayCount, DXGI_FORMAT Format
    );

    // Get pre-created CPU-visible descriptor handles
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const 
    {
        return m_SRVHandle; 
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV(void) const 
    {
        return m_RTVHandle; 
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const 
    { 
        return m_UAVHandle[0]; 
    }

    void SetClearColor(custom::Color ClearColor) 
    { 
        m_clearColor = ClearColor; 
    }

    void SetMsaaMode(uint32_t NumColorSamples, uint32_t NumCoverageSamples)
    {
        ASSERT(NumCoverageSamples >= NumColorSamples);
        m_fragmentCount = NumColorSamples;
        m_sampleCount = NumCoverageSamples;
    }

    custom::Color GetClearColor(void) const 
    { 
        return m_clearColor; 
    }

    void GenerateMipMaps(custom::CommandContext& Context);

protected:
    D3D12_RESOURCE_FLAGS CombineResourceFlags(void) const
    {
        D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

		if (Flags == D3D12_RESOURCE_FLAG_NONE && m_fragmentCount == 1)
		{
			Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

        return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
    }

    // Compute the number of texture levels needed to reduce to 1x1.  This uses
    // _BitScanReverse to find the highest set bit.  Each dimension reduces by
    // half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
    // as the dimension 511 (0x1FF).
    static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
    {
        uint32_t HighBit;
        _BitScanReverse((unsigned long*)&HighBit, Width | Height);
        return HighBit + 1;
    }
    void createResourceViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);

protected:
    custom::Color m_clearColor;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
    uint32_t m_numMipMaps; // number of texture sublevels
    uint32_t m_fragmentCount;
    uint32_t m_sampleCount;
};
