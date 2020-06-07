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
        Texture(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
            : m_hCpuDescriptorHandle(Handle)
        {
        }

        // Create a 1-level 2D texture
        void Create(size_t Pitch, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData);

        void Create(size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData)
        {
            Create(Width, Width, Height, Format, InitData);
        }

        void CreateTGAFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);
        bool CreateDDSFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);
        void CreatePIXImageFromMemory(const void* memBuffer, size_t fileSize);

        virtual void Destroy() override
        {
            GPUResource::Destroy();
            // This leaks descriptor handles.  We should really give it back to be reused.
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