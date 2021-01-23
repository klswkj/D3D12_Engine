#include "stdafx.h"
#include "Texture.h"
#include "Device.h"
#include "Graphics.h"
#include "CommandContext.h"
#include "DDSTextureLoader.cpp" // D3D12
#include "WICTextureLoader.h"   // D3D12

namespace custom
{
    static UINT BytesPerPixel(DXGI_FORMAT Format)
    {
        return (UINT)::BitsPerPixel(Format) / 8;
    };

    void Texture::Create(size_t Pitch, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData)
    {
        m_currentState = D3D12_RESOURCE_STATE_COPY_DEST;

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

        D3D12_HEAP_PROPERTIES HeapProperties;
        HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProperties.CreationNodeMask = 1;
        HeapProperties.VisibleNodeMask = 1;

        ASSERT_HR
        (
            device::g_pDevice->CreateCommittedResource
            (
                &HeapProperties, D3D12_HEAP_FLAG_NONE, &texDesc,
                m_currentState, nullptr, IID_PPV_ARGS(&m_pResource)
            )
        );

        m_pResource->SetName(L"Texture");

        D3D12_SUBRESOURCE_DATA texResource;
        texResource.pData = InitialData;
        texResource.RowPitch = Pitch * custom::BytesPerPixel(Format);
        texResource.SlicePitch = texResource.RowPitch * Height;

        CommandContext::InitializeTexture(*this, 1, &texResource);

		if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hCpuDescriptorHandle = graphics::g_DescriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
        device::g_pDevice->CreateShaderResourceView(m_pResource, nullptr, m_hCpuDescriptorHandle);
    }

    void Texture::CreateTGAFromMemory(const void* _filePtr, size_t, bool bStandardRGB)
    {
        const uint8_t* filePtr = (const uint8_t*)_filePtr;

		{
			// Skip first two bytes
			filePtr += 2;

			/*uint8_t imageTypeCode =*/
			*filePtr++;

			// Ignore another 9 bytes
			filePtr += 9;
		}
        uint16_t imageWidth = *(uint16_t*)filePtr;
        filePtr += sizeof(uint16_t);
        uint16_t imageHeight = *(uint16_t*)filePtr;
        filePtr += sizeof(uint16_t);
        uint8_t bitCount = *filePtr++;

        // Ignore another byte
        ++filePtr;

        uint32_t* formattedData = new uint32_t[imageWidth * imageHeight];
        uint32_t* iter = formattedData;

        uint8_t numChannels = bitCount / 8;
        uint32_t numBytes = imageWidth * imageHeight * numChannels;

        switch (numChannels)
        {
        case 3:
            for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 3)
            {
                *iter++ = 0xff000000 | filePtr[0] << 16 | filePtr[1] << 8 | filePtr[2];
                filePtr += 3;
            }
            break;
        case 4:
            for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 4)
            {
                *iter++ = filePtr[3] << 24 | filePtr[0] << 16 | filePtr[1] << 8 | filePtr[2];
                filePtr += 4;
            }
            break;
        default:
            ASSERT(numChannels == 3 || numChannels == 4, "Invalid Channel.");
        }

        Create(imageWidth, imageHeight, bStandardRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, formattedData);

        delete[] formattedData;
    }

    bool Texture::CreateDDSFromMemory(const void* filePtr, size_t fileSize, bool bStandardRGB)
    {
		if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hCpuDescriptorHandle = graphics::g_DescriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

        HRESULT hr = CreateDDSTextureFromMemory
        (
            device::g_pDevice,
            (const uint8_t*)filePtr, fileSize, 0, bStandardRGB, &m_pResource, m_hCpuDescriptorHandle, nullptr
        );

        ASSERT_HR(hr);

        return SUCCEEDED(hr);
    }

    void Texture::CreatePIXImageFromMemory(const void* memBuffer, size_t fileSize)
    {
        struct Header
        {
            DXGI_FORMAT Format;
            uint32_t Pitch;
            uint32_t Width;
            uint32_t Height;
        };
        const Header& header = *(Header*)memBuffer;

        ASSERT
        (
            fileSize >= header.Pitch * custom::BytesPerPixel(header.Format) * header.Height + sizeof(Header),
            "Raw PIX image dump has an invalid file size"
        );

        Create(header.Pitch, header.Width, header.Height, header.Format, (uint8_t*)memBuffer + sizeof(Header));
    }

    bool Texture::CreateWICFromMemory(const std::wstring& fileName)
    {
        m_currentState = D3D12_RESOURCE_STATE_COPY_DEST;

        D3D12_SUBRESOURCE_DATA subresource;
        std::unique_ptr<uint8_t[]> wicData; // using to subResource.pData

        HRESULT hr = DirectX::LoadWICTextureFromFile
        (
            device::g_pDevice, fileName.c_str(), &m_pResource,
            wicData, subresource,
            size_t(D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        );

        ASSERT_HR(hr, "Invalid Creating WIC Request from ", fileName, ".");

        CommandContext::InitializeTexture(*this, 1, &subresource);

        if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hCpuDescriptorHandle = graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
        device::g_pDevice->CreateShaderResourceView(m_pResource, nullptr, m_hCpuDescriptorHandle);
#if(_DEBUG)
        std::wstring normalizedwString = _NormalizeWString(fileName);
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