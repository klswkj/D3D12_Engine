#include "stdafx.h"
#include "Device.h"
#include "Graphics.h"
#include "DepthBuffer.h"

void DepthBuffer::Create
(
    const std::wstring& wName, 
    const uint32_t width, 
    const uint32_t height, 
    const DXGI_FORMAT format
)
{
    D3D12_RESOURCE_DESC ResourceDesc = Texture2DResourceDescriptor(width, height, 1u, 1u, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format            = format;

    CreateTextureCommittedResource(device::g_pDevice, wName, ResourceDesc, &ClearValue);
    createDSV(device::g_pDevice, format);
}

void DepthBuffer::CreateSamples
(
    const std::wstring& wName, 
    const uint32_t width,
    const uint32_t height, 
    const uint32_t samples,
    const DXGI_FORMAT format
)
{
    D3D12_RESOURCE_DESC ResourceDesc = Texture2DResourceDescriptor(width, height, 1, 1, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ResourceDesc.SampleDesc.Count    = samples;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format            = format;

    CreateTextureCommittedResource(device::g_pDevice, wName, ResourceDesc, &ClearValue);
    createDSV(device::g_pDevice, format);
}

void DepthBuffer::createDSV(ID3D12Device* const pDevice, const DXGI_FORMAT format)
{
    ID3D12Resource* Resource = m_pResource;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = GetDSVFormat(format);
    m_Format = dsvDesc.Format;

    if (Resource->GetDesc().SampleDesc.Count == 1U)
    {
        dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0U;
    }
    else
    {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    }

    if (m_hDSV[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_hDSV[0] = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_hDSV[1] = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    pDevice->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[0]);

    dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    pDevice->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[1]);

    DXGI_FORMAT stencilReadFormat = GetStencilFormat(format);

    if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
    {
        if (m_hDSV[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        {
            m_hDSV[2] = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            m_hDSV[3] = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }

        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
        pDevice->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[2]);

        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
        pDevice->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[3]);
    }
    else
    {
        m_hDSV[2] = m_hDSV[0];
        m_hDSV[3] = m_hDSV[1];
    }

	if (m_hDepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		m_hDepthSRV = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = GetDepthFormat(format);


    if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1U;
    }
    else
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }

    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    pDevice->CreateShaderResourceView(Resource, &SRVDesc, m_hDepthSRV);

    if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
    {
		if (m_hStencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hStencilSRV = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

        SRVDesc.Format = stencilReadFormat;
        pDevice->CreateShaderResourceView(Resource, &SRVDesc, m_hStencilSRV);
    }
}
/*
https://stackoverflow.com/questions/38933565/which-format-to-use-for-a-shader-resource-view-into-depth-stencil-buffer-resourc

Resource as R24G8_TYPELESS
DSV as D24_UNORM_S8_UINT
SRV as R24_UNORM_X8_TYPELESS

DXGI_FORMAT_D24_UNORM_S8_UINT
D3D12 ERROR: ID3D12Device::CreateShaderResourceView: 
The Plane Slice 0 cannot be used when the resource format is R24G8_TYPELESS and the view format is X24_TYPELESS_G8_UINT.
See documentation for the set of valid view format names for this resource format, 
determining which how the resource (or part of it) will appear to shader. 
[ STATE_CREATION ERROR #29: CREATESHADERRESOURCEVIEW_INVALIDVIDEOPLANESLICE]
*/
