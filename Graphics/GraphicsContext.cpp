#include "stdafx.h"
#include "Graphics.h"
#include "GraphicsContext.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "CommandSignature.h"

namespace custom
{
    // TODO : Eliminate Overloading.

    inline void GraphicsContext::SetRootSignature(const RootSignature& RootSig)
    {
        if (RootSig.GetSignature() == m_pCurrentGraphicsRootSignature)
            return;

        m_commandList->SetGraphicsRootSignature(m_pCurrentGraphicsRootSignature = RootSig.GetSignature());

        m_DynamicViewDescriptorHeap.ParseGraphicsRootSignature(RootSig);
        m_DynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(RootSig);
    }

    void GraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV)
    {
        m_commandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV);
    }

    void GraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
    {
        m_commandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
    }

    void GraphicsContext::BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
    {
        m_commandList->BeginQuery(QueryHeap, Type, HeapIndex);
    }

    void GraphicsContext::EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
    {
        m_commandList->EndQuery(QueryHeap, Type, HeapIndex);
    }

    void GraphicsContext::ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
    {
        m_commandList->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);
    }

    void GraphicsContext::ClearUAV(UAVBuffer& Target)
    {
        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV 
        // (because it essentially runs a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
        const UINT ClearColor[4] = {};
        m_commandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
    }

    void GraphicsContext::ClearUAV(ColorBuffer& Target)
    {
        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV 
        // (because it essentially runs a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
        CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

        const float* ClearColor = Target.GetClearColor().GetPtr();
        m_commandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
    }

    void GraphicsContext::ClearColor(ColorBuffer& Target)
    {
        m_commandList->ClearRenderTargetView(Target.GetRTV(), Target.GetClearColor().GetPtr(), 0, nullptr);
    }

    void GraphicsContext::ClearDepth(DepthBuffer& Target)
    {
        m_commandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
    }

    void GraphicsContext::ClearStencil(DepthBuffer& Target)
    {
        m_commandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
    }

    void GraphicsContext::ClearDepthAndStencil(DepthBuffer& Target)
    {
        m_commandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
    }

    void GraphicsContext::SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
    {
        ASSERT(rect.left < rect.right&& rect.top < rect.bottom);
        m_commandList->RSSetViewports(1, &vp);
        m_commandList->RSSetScissorRects(1, &rect);
    }

    void GraphicsContext::SetViewport(const D3D12_VIEWPORT& vp)
    {
        m_commandList->RSSetViewports(1, &vp);
    }

    void GraphicsContext::SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth, FLOAT maxDepth)
    {
        D3D12_VIEWPORT vp;
        vp.Width = w;
        vp.Height = h;
        vp.MinDepth = minDepth;
        vp.MaxDepth = maxDepth;
        vp.TopLeftX = x;
        vp.TopLeftY = y;
        m_commandList->RSSetViewports(1, &vp);
    }

    void GraphicsContext::SetScissor(const D3D12_RECT& rect)
    {
        ASSERT(rect.left < rect.right&& rect.top < rect.bottom);
        m_commandList->RSSetScissorRects(1, &rect);
    }

    inline void GraphicsContext::SetScissor(LONG left, LONG top, LONG right, LONG bottom)
    {
        SetScissor(CD3DX12_RECT(left, top, right, bottom));
    }

    inline void GraphicsContext::SetViewportAndScissor(LONG x, LONG y, LONG w, LONG h)
    {
        SetViewport((float)x, (float)y, (float)w, (float)h);
        SetScissor(x, y, x + w, y + h);
    }

    void GraphicsContext::SetConstants(UINT RootIndex, uint32_t size, ...)
    {
        va_list Args;
        va_start(Args, size);

        UINT val;

        for (UINT i = 0; i < size; ++i)
        {
            val = va_arg(Args, UINT);
            m_commandList->SetGraphicsRoot32BitConstant(RootIndex, val, i);
        }
    }

    inline void GraphicsContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16));
        LinearBuffer cb = m_CPULinearAllocator.Allocate(BufferSize);
        //SIMDMemCopy(cb.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
        memcpy(cb.pData, BufferData, BufferSize);
        m_commandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GPUAddress);
    }

    inline void GraphicsContext::SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VertexData)
    {
        ASSERT(VertexData != nullptr && HashInternal::IsAligned(VertexData, 16));

        size_t BufferSize = HashInternal::AlignUp(NumVertices * VertexStride, 16);
        LinearBuffer vb = m_CPULinearAllocator.Allocate(BufferSize);

        SIMDMemCopy(vb.pData, VertexData, BufferSize >> 4);

        D3D12_VERTEX_BUFFER_VIEW VBView;
        VBView.BufferLocation = vb.GPUAddress;
        VBView.SizeInBytes = (UINT)BufferSize;
        VBView.StrideInBytes = (UINT)VertexStride;

        m_commandList->IASetVertexBuffers(Slot, 1, &VBView);
    }

    inline void GraphicsContext::SetDynamicIB(size_t IndexCount, const uint16_t* IndexData)
    {
        ASSERT(IndexData != nullptr && HashInternal::IsAligned(IndexData, 16));

        size_t BufferSize = HashInternal::AlignUp(IndexCount * sizeof(uint16_t), 16);
        LinearBuffer ib = m_CPULinearAllocator.Allocate(BufferSize);

        SIMDMemCopy(ib.pData, IndexData, BufferSize >> 4);

        D3D12_INDEX_BUFFER_VIEW IBView;
        IBView.BufferLocation = ib.GPUAddress;
        IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
        IBView.Format = DXGI_FORMAT_R16_UINT;

        m_commandList->IASetIndexBuffer(&IBView);
    }

    inline void GraphicsContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16));
        LinearBuffer cb = m_CPULinearAllocator.Allocate(BufferSize);
        SIMDMemCopy(cb.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
        m_commandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GPUAddress);
    }

    inline void GraphicsContext::SetBufferSRV(UINT RootIndex, const UAVBuffer& SRV, UINT64 Offset)
    {
        ASSERT((SRV.m_currentState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
        m_commandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
    }

    inline void GraphicsContext::SetBufferUAV(UINT RootIndex, const UAVBuffer& UAV, UINT64 Offset)
    {
        ASSERT((UAV.m_currentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
        m_commandList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
    }

    inline void GraphicsContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
    }

    inline void GraphicsContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        m_DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    inline void GraphicsContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    {
        SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
    }

    inline void GraphicsContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
    {
        m_DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
    }

    inline void GraphicsContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        m_commandList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
    }

    inline void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
    {
        m_commandList->IASetIndexBuffer(&IBView);
    }

    inline void GraphicsContext::SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
    {
        SetVertexBuffers(Slot, 1, &VBView);
    }

    inline void GraphicsContext::SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
    {
        m_commandList->IASetVertexBuffers(StartSlot, Count, VBViews);
    }

    inline void GraphicsContext::Draw(UINT VertexCount, UINT VertexStartOffset)
    {
        DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
    }

    inline void GraphicsContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
    {
        DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
    }

    inline void GraphicsContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
        UINT StartVertexLocation, UINT StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
        m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
        m_commandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    inline void GraphicsContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
        INT BaseVertexLocation, UINT StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
        m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
        m_commandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    inline void GraphicsContext::ExecuteIndirect
    (
        CommandSignature& CommandSig,
        UAVBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
        uint32_t MaxCommands, UAVBuffer* CommandCounterBuffer, uint64_t CounterOffset
    )
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
        m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
        m_commandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
            ArgumentBuffer.GetResource(), ArgumentStartOffset,
            CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
    }

    inline void GraphicsContext::DrawIndirect(UAVBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
    {
        ExecuteIndirect(graphics::DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
    }
}