#include "stdafx.h"
#include "Texture.h"
#include "Device.h"
#include "Graphics.h"
#include "CopyContext.h"
#include "DDSTextureLoader.cpp" // D3D12
#include "WICTextureLoader.h"   // D3D12

namespace custom
{
    static UINT BytesPerPixel(DXGI_FORMAT Format)
    {
        return (UINT)::BitsPerPixel(Format) / 8;
    };

    void Texture::CreateCommittedTexturePrivate
    (
        const size_t Pitch, 
        const size_t Width, const size_t Height, 
        const DXGI_FORMAT Format, const void* const InitialData
    )
    {
        m_numSubResource = 1u;
        m_currentStates.resize(1ul, D3D12_RESOURCE_STATE_COPY_DEST);
        m_pendingStates.resize(1ul, D3D12_RESOURCE_STATES(-1));

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width               = Width;
        texDesc.Height              = (UINT)Height;
        texDesc.DepthOrArraySize    = 1;
        texDesc.MipLevels           = 1;
        texDesc.Format              = Format;
        texDesc.SampleDesc.Count    = 1;
        texDesc.SampleDesc.Quality  = 0;
        texDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES HeapProperties = {};
        HeapProperties.Type                  = D3D12_HEAP_TYPE_DEFAULT;
        HeapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProperties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProperties.CreationNodeMask      = 0x1;
        HeapProperties.VisibleNodeMask       = 0x1;

        ASSERT_HR
        (
            device::g_pDevice->CreateCommittedResource
            (
                &HeapProperties, D3D12_HEAP_FLAG_NONE, &texDesc,
                m_currentStates.front(), nullptr, IID_PPV_ARGS(&m_pResource)
            )
        );

        m_pResource->SetName(L"Texture");

        D3D12_SUBRESOURCE_DATA texResource = {};
        texResource.pData      = InitialData;
        texResource.RowPitch   = Pitch * custom::BytesPerPixel(Format);
        texResource.SlicePitch = texResource.RowPitch * Height;

        CopyContext::InitializeTexture(*this, 1, &texResource);

		if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hCpuDescriptorHandle = graphics::g_DescriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
        device::g_pDevice->CreateShaderResourceView(m_pResource, nullptr, m_hCpuDescriptorHandle);
    }

    void Texture::CreateTGAFromMemory(const void* const pFile, size_t, const bool bStandardRGB)
    {
        const uint8_t* FilePtr = (const uint8_t*)pFile;

		{
			// Skip first two bytes
			FilePtr += 2;

			/*uint8_t imageTypeCode =*/
			*FilePtr++;

			// Ignore another 9 bytes
			FilePtr += 9;
		}

        uint16_t ImageWidth = *(uint16_t*)FilePtr;
        FilePtr += sizeof(uint16_t);
        uint16_t ImageHeight = *(uint16_t*)FilePtr;
        FilePtr += sizeof(uint16_t);
        uint8_t BitCount = *FilePtr++;

        // Ignore another byte
        ++FilePtr;

        uint32_t* pFormattedData = new uint32_t[(uint64_t)ImageWidth * (uint64_t)ImageHeight];
        uint32_t* pIter = pFormattedData;

        uint8_t NumChannels = BitCount / 8u;
        uint32_t NumBytes = ImageWidth * ImageHeight * NumChannels;

        switch (NumChannels)
        {
        case 3:
            for (uint32_t byteIdx = 0u; byteIdx < NumBytes; byteIdx += 3u)
            {
                *pIter++ = 0xff000000 | FilePtr[0] << 16 | FilePtr[1] << 8 | FilePtr[2];
                FilePtr += 3;
            }
            break;
        case 4:
            for (uint32_t byteIdx = 0u; byteIdx < NumBytes; byteIdx += 4u)
            {
                *pIter++ = FilePtr[3] << 24 | FilePtr[0] << 16 | FilePtr[1] << 8 | FilePtr[2];
                FilePtr += 4;
            }
            break;
        default:
            ASSERT(NumChannels == 3 || NumChannels == 4, "Invalid Channel.");
        }

        CreateCommittedTexture(ImageWidth, ImageHeight, bStandardRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, pFormattedData);

        delete[] pFormattedData;
    }

    bool Texture::CreateDDSFromMemory(const void* const pFile, const size_t fileSize, const bool bStandardRGB)
    {
		if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hCpuDescriptorHandle = graphics::g_DescriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

        HRESULT hr = CreateDDSTextureFromMemory
        (
            device::g_pDevice,
            (const uint8_t*)pFile, fileSize, 0, bStandardRGB, &m_pResource, m_hCpuDescriptorHandle, nullptr
        );

        ASSERT_HR(hr);

        return SUCCEEDED(hr);
    }

    void Texture::CreatePIXImageFromMemory(const void* const pMemBuffer, const size_t fileSize)
    {
        struct Header
        {
            DXGI_FORMAT Format;
            uint32_t Pitch;
            uint32_t Width;
            uint32_t Height;
        };
        const Header& header = *(Header*)pMemBuffer;

        // TODO 0 : fix ASSERT condition.
        ASSERT
        (
             (size_t)header.Pitch * (size_t)custom::BytesPerPixel(header.Format) * (size_t)header.Height + sizeof(Header) <= fileSize,
            "Raw PIX image dump has an invalid file size"
        );

        CreateCommittedTexturePrivate(header.Pitch, header.Width, header.Height, header.Format, (uint8_t*)pMemBuffer + sizeof(Header));
    }

    bool Texture::CreateWICFromMemory(const std::wstring& szFileName)
    {
        D3D12_SUBRESOURCE_DATA SubResource;
        std::unique_ptr<uint8_t[]> DecodedWICData; // using to subResource.pData

        HRESULT hr = DirectX::LoadWICTextureFromFile
        (
            device::g_pDevice, szFileName.c_str(), &m_pResource,
            DecodedWICData, SubResource,
            size_t(D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        );

        ASSERT_HR(hr, "Invalid Creating WIC Request from ", szFileName, ".");
        // GetRequiredIntermediateSize

        D3D12_RESOURCE_DESC desc = m_pResource->GetDesc();
        UINT NumSubResource = (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? desc.DepthOrArraySize * desc.MipLevels : desc.MipLevels;
        
        m_numSubResource = NumSubResource;
        m_currentStates.resize((size_t)NumSubResource, D3D12_RESOURCE_STATE_COPY_DEST);
        m_pendingStates.resize((size_t)NumSubResource, D3D12_RESOURCE_STATES(-1));

        CopyContext::InitializeTexture(*this, NumSubResource, &SubResource);

        if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hCpuDescriptorHandle = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
        device::g_pDevice->CreateShaderResourceView(m_pResource, nullptr, m_hCpuDescriptorHandle);
#if(_DEBUG)
        std::wstring normalizedwString = _NormalizeWString(szFileName);
        normalizedwString += L"_ManagedTexture"; // will be removed.
        m_pResource->SetName(normalizedwString.c_str());
#endif
        return SUCCEEDED(hr);
    }
}

/*
void Texture::Create(size_t Pitch, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData)
{
    m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = Width;
    texDesc.Height = (UINT)Height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = Format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        m_UsageState, nullptr, MY_IID_PPV_ARGS(m_pResource.ReleaseAndGetAddressOf())));

    m_pResource->SetName(L"Texture");

    D3D12_SUBRESOURCE_DATA texResource;
    texResource.pData = InitialData;
    texResource.RowPitch = Pitch * BytesPerPixel(Format);
    texResource.SlicePitch = texResource.RowPitch * Height;

    CommandContext::InitializeTexture(*this, 1, &texResource);

    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hCpuDescriptorHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateShaderResourceView(m_pResource.Get(), nullptr, m_hCpuDescriptorHandle);
}
*/