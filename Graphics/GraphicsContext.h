#pragma once
#include "stdafx.h"
#include "CommandContext.h"

namespace custom
{
    class GraphicsContext : public CommandContext
    {
    public:
        static GraphicsContext& Begin(const std::wstring& ID = L"")
        {
            return CommandContext::Begin(ID).GetGraphicsContext();
        }

        void ClearUAV(UAVBuffer& Target);
        void ClearUAV(ColorBuffer& Target);
        void ClearColor(ColorBuffer& Target);
        void ClearDepth(DepthBuffer& Target);
        void ClearStencil(DepthBuffer& Target);
        void ClearDepthAndStencil(DepthBuffer& Target);

        void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
        void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
        void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset);

        void SetRootSignature(const RootSignature& RootSig);

        void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
        void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
        void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV) { SetRenderTargets(1, &RTV); }
        void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(1, &RTV, DSV); }
        void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(0, nullptr, DSV); }

        void SetViewport(const D3D12_VIEWPORT& Viewport);
        void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
        void SetScissor(const D3D12_RECT& Rect);
        void SetScissor(LONG left, LONG top, LONG right, LONG bottom);
        void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
        void SetViewportAndScissor(LONG x, LONG y, LONG w, LONG h);
        void SetStencilRef(UINT StencilRef) { m_commandList->OMSetStencilRef(StencilRef); }
        void SetBlendFactor(custom::Color BlendFactor) { m_commandList->OMSetBlendFactor(BlendFactor.GetPtr()); }
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology) { m_commandList->IASetPrimitiveTopology(Topology); }

        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants) { m_commandList->SetComputeRoot32BitConstants(RootIndex, NumConstants, pConstants, 0); }
        void SetConstants(UINT RootIndex, uint32_t size, ...);

        void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV) { m_commandList->SetComputeRootConstantBufferView(RootIndex, CBV); }
        void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData);
        void SetBufferSRV(UINT RootIndex, const UAVBuffer& SRV, UINT64 Offset = 0);
        void SetBufferUAV(UINT RootIndex, const UAVBuffer& UAV, UINT64 Offset = 0);
        void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView);
        void SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView);
        void SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[]);
        void SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData);
        void SetDynamicIB(size_t IndexCount, const uint16_t* IBData);
        void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);

        void Draw(UINT VertexCount, UINT VertexStartOffset = 0);
        void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
        void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
            UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);
        void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
            INT BaseVertexLocation, UINT StartInstanceLocation);
        void DrawIndirect(UAVBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);
        void ExecuteIndirect
        (
            CommandSignature& CommandSig, UAVBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
            uint32_t MaxCommands = 1, UAVBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0
        );
    private:
        
    };
}