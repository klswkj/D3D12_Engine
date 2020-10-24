#include "stdafx.h"
#include "ColorBuffer.h"
#include "device.h"
#include "graphics.h"
#include "CommandContext.h"
#include "ComputeContext.h"
#include "PSO.h"

void ColorBuffer::createResourceViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
{
    ASSERT(ArraySize == 1 || NumMips == 1, "We don't support auto-mips on texture arrays");

    m_numMipMaps = NumMips - 1;

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

    RTVDesc.Format = Format;
    UAVDesc.Format = GetUAVFormat(Format);
    SRVDesc.Format = Format;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (1 < ArraySize)
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        RTVDesc.Texture2DArray.MipSlice = 0;
        RTVDesc.Texture2DArray.FirstArraySlice = 0;
        RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UAVDesc.Texture2DArray.MipSlice = 0;
        UAVDesc.Texture2DArray.FirstArraySlice = 0;
        UAVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        SRVDesc.Texture2DArray.MipLevels = NumMips;
        SRVDesc.Texture2DArray.MostDetailedMip = 0;
        SRVDesc.Texture2DArray.FirstArraySlice = 0;
        SRVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;
    }
    else if (1 < m_fragmentCount)
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        RTVDesc.Texture2D.MipSlice = 0;

        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        UAVDesc.Texture2D.MipSlice = 0;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = NumMips;
        SRVDesc.Texture2D.MostDetailedMip = 0;
    }

    if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_RTVHandle = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_SRVHandle = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    ID3D12Resource* Resource = m_pResource;

    // Create the render target view
    Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle);

    // Create the shader resource view
    Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);

    if (m_fragmentCount > 1)
        return;

    // Create the UAVs for each mip level (RWTexture2D)
    for (uint32_t i = 0; i < NumMips; ++i)
    {
        if (m_UAVHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            m_UAVHandle[i] = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, m_UAVHandle[i]);

        UAVDesc.Texture2D.MipSlice++;
    }
}

void ColorBuffer::CreateFromSwapChain(const std::wstring& Name, ID3D12Resource* BaseResource)
{
    CopyResource(device::g_pDevice, Name, BaseResource, D3D12_RESOURCE_STATE_PRESENT);

    //m_UAVHandle[0] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    //Graphics::g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, nullptr, m_UAVHandle[0]);

    m_RTVHandle = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    device::g_pDevice->CreateRenderTargetView(m_pResource, nullptr, m_RTVHandle);
}

void ColorBuffer::Create
(
    const std::wstring& Name, uint32_t Width, uint32_t Height, 
    uint32_t NumMips, DXGI_FORMAT Format
)
{
    NumMips = (NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);
    D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
    D3D12_RESOURCE_DESC ResourceDesc = Texture2DResourceDescriptor(Width, Height, 1, NumMips, Format, Flags);

    ResourceDesc.SampleDesc.Count = m_fragmentCount;
    ResourceDesc.SampleDesc.Quality = 0;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    ClearValue.Color[0] = m_clearColor.R();
    ClearValue.Color[1] = m_clearColor.G();
    ClearValue.Color[2] = m_clearColor.B();
    ClearValue.Color[3] = m_clearColor.A();

    CreateTextureResource(device::g_pDevice, Name, ResourceDesc, ClearValue);
    createResourceViews(device::g_pDevice, Format, 1, NumMips);
}

void ColorBuffer::CreateArray
(
    const std::wstring& Name, uint32_t Width, uint32_t Height, 
    uint32_t ArrayCount, DXGI_FORMAT Format
)
{
    D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
    D3D12_RESOURCE_DESC ResourceDesc = Texture2DResourceDescriptor(Width, Height, ArrayCount, 1, Format, Flags);

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    ClearValue.Color[0] = m_clearColor.R();
    ClearValue.Color[1] = m_clearColor.G();
    ClearValue.Color[2] = m_clearColor.B();
    ClearValue.Color[3] = m_clearColor.A();

    CreateTextureResource(device::g_pDevice, Name, ResourceDesc, ClearValue);
    createResourceViews(device::g_pDevice, Format, ArrayCount, 1);
}

void ColorBuffer::GenerateMipMaps(custom::CommandContext& BaseContext)
{
    if (m_numMipMaps == 0)
	{
		return;
	}

    custom::ComputeContext& Context = BaseContext.GetComputeContext();

    Context.SetRootSignature(graphics::g_GenerateMipsRootSignature);

    Context.TransitionResource(*this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Context.SetDynamicDescriptor(1, 0, m_SRVHandle);

    for (uint32_t TopMip = 0; TopMip < m_numMipMaps; )
    {
        uint32_t SrcWidth = m_width >> TopMip;
        uint32_t SrcHeight = m_height >> TopMip;
        uint32_t DstWidth = SrcWidth >> 1;
        uint32_t DstHeight = SrcHeight >> 1;

        // Determine if the first downsample is more than 2:1.  This happens whenever
        // the source width or height is odd.
        uint32_t NonPowerOfTwo = (SrcWidth & 1) | (SrcHeight & 1) << 1;
        if (m_format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
        {
            Context.SetPipelineState(graphics::g_GenerateMipsGammaComputePSO[NonPowerOfTwo]);
        }
        else
        {
            Context.SetPipelineState(graphics::g_GenerateMipsLinearComputePSO[NonPowerOfTwo]);
        }
        // We can downsample up to four times, but if the ratio between levels is not
        // exactly 2:1, we have to shift our blend weights, which gets complicated or
        // expensive.  Maybe we can update the code later to compute sample weights for
        // each successive downsample.  We use _BitScanForward to count number of zeros
        // in the low bits.  Zeros indicate we can divide by two without truncating.
        uint32_t AdditionalMips;
        size_t BitTarget = (DstWidth == 1 ? DstHeight : DstWidth) | (DstHeight == 1 ? DstWidth : DstHeight);

        _BitScanForward
        (
            (unsigned long*)&AdditionalMips,
            BitTarget
        );
        uint32_t NumMips = 1 + (3 < AdditionalMips ? 3 : AdditionalMips);
        if (m_numMipMaps < TopMip + NumMips)
        {
            NumMips = m_numMipMaps - TopMip;
        }
        // These are clamped to 1 after computing additional mips because clamped
        // dimensions should not limit us from downsampling multiple times.  (E.g.
        // 16x1 -> 8x1 -> 4x1 -> 2x1 -> 1x1.)
        if (DstWidth == 0)
        {
            DstWidth = 1;
        }
		if (DstHeight == 0)
		{
			DstHeight = 1;
		}

        float temp1 = 1.0f / DstWidth;
        float temp2 = 1.0f / DstHeight;

        Context.SetConstants(0, TopMip, NumMips, *reinterpret_cast<UINT*>(&temp1), *reinterpret_cast<UINT*>(&temp2));
        Context.SetDynamicDescriptors(2, 0, NumMips, m_UAVHandle + TopMip + 1);
        Context.Dispatch2D(DstWidth, DstHeight);

        Context.InsertUAVBarrier(*this);

        TopMip += NumMips;
    }

    Context.TransitionResource
    (
        *this, 
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );
}
