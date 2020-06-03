#pragma once
#include "stdafx.h"

namespace custom
{
    class Texture : public GPUResource
    {
        friend class CommandContext;

    public:
        Texture(ID3D12Device* m_Device, CommandContext& CommandContext, DescriptorHeapManager& DescriptorHeapManager)
            : g_pDevice(m_Device), m_rCommandContext(CommandContext), m_rDescriptorHeapManager(DescriptorHeapManager)
        { 
            m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; 
        }
        Texture(CommandContext& CommandContext, DescriptorHeapManager& DescriptorHeapManager, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
            : m_rCommandContext(CommandContext), m_rDescriptorHeapManager(DescriptorHeapManager), m_hCpuDescriptorHandle(Handle)
        {
            g_pDevice = nullptr;
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
        ID3D12Device*               g_pDevice;
        CommandContext&             m_rCommandContext;
        DescriptorHeapManager&      m_rDescriptorHeapManager;
        D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
    };

}