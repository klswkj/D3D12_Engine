#pragma once
#include "stdafx.h"
#include "GPUResource.h"

namespace custom
{
    class CommandContext;
}

namespace custom
{
    class Texture : public GPUResource
    {
        friend class CommandContext;

    public:
        Texture()
        { 
            m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; 
        }
        Texture(const D3D12_CPU_DESCRIPTOR_HANDLE handle)
            : m_hCpuDescriptorHandle(handle)
        {
        }
        virtual ~Texture() = default;

        // Create a 1-level 2D texture
        void CreateCommittedTexturePrivate(const size_t pitch, const size_t width, const size_t height, const DXGI_FORMAT format, const void* const pInitData);

        void CreateCommittedTexture(const size_t width, const size_t height, const DXGI_FORMAT format, const void* const pInitData)
        {
            CreateCommittedTexturePrivate(width, width, height, format, pInitData);
        }

        void CreateTGAFromMemory(const void* memBuffer, size_t fileSize, bool bStandardRGB);
        bool CreateDDSFromMemory(const void* memBuffer, size_t fileSize, bool bStandardRGB);
        void CreatePIXImageFromMemory(const void* memBuffer, size_t fileSize);
        bool CreateWICFromMemory(const std::wstring& fileName);

        virtual void Destroy() override
        {
            GPUResource::Destroy();
            // This leaks descriptor handles. 
            // We should really give it back to be reused.
            m_hCpuDescriptorHandle.ptr = 0;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const 
        { 
            return m_hCpuDescriptorHandle; 
        }

        bool operator!() 
        { 
            return (m_hCpuDescriptorHandle.ptr == 0); 
        }

    protected:
        D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
    };
}