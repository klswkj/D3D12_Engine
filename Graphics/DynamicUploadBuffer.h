#pragma once
#include "stdafx.h"

class DynamicUploadBuffer
{
public:
    DynamicUploadBuffer() 
        : m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL), m_CpuVirtualAddress(nullptr) 
    {
    }
    ~DynamicUploadBuffer() 
    { 
        Destroy(); 
    }

    void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize);
    void Destroy();

    // Map a CPU-visible pointer to the buffer memory.  You probably don't want to leave a lot of
    // memory (100s of MB) mapped this way, so you have the option of unmapping it.
    void* Map();
    void Unmap();

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView(uint32_t NumVertices, uint32_t Stride, uint32_t Offset = 0) const;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView(uint32_t NumIndices, bool _32bit, uint32_t Offset = 0) const;
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuPointer(uint32_t Offset = 0) const
    {
        return m_GpuVirtualAddress + Offset;
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
    D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
    void* m_CpuVirtualAddress{ nullptr };
    // VertexBuffer 테스트하는 곳에서 코드 업데이트 아직 안함.
};
