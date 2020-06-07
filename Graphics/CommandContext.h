#pragma once
#include "stdafx.h"
#include "Color.h"
#include "GPUResource.h"
#include "UAVBuffer.h"
#include "LinearAllocator.h"
#include "DynamicDescriptorHeap.h"

class CommandContextManager;
class PixelBuffer;

namespace custom
{
    class GraphicsContext;
    class ComputeContext;

    class CommandContext
    {
        friend CommandContextManager;
    private:
        CommandContext(D3D12_COMMAND_LIST_TYPE Type);
        CommandContext(const CommandContext&) = delete;
        CommandContext& operator=(const CommandContext&) = delete;

        void Reset(void);

    public:

        ~CommandContext(void);

        static void DestroyAllContexts(void);

        static CommandContext& Begin(const std::wstring ID = L"");

        // Flush existing commands to the GPU but keep the context alive
        uint64_t Flush(bool WaitForCompletion = false);

        // Flush existing commands and release the current context
        uint64_t Finish(bool WaitForCompletion = false);

        // Prepare to render by reserving a command list and command allocator
        void Initialize(void);

        GraphicsContext& GetGraphicsContext() 
        {
            ASSERT(m_type != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics");
            return reinterpret_cast<GraphicsContext&>(*this);
        }

        ComputeContext& GetComputeContext() 
        {
            return reinterpret_cast<ComputeContext&>(*this);
        }

        ID3D12GraphicsCommandList* GetCommandList() 
        {
            return m_commandList;
        }

        void CopyBuffer(GPUResource& Dest, GPUResource& Src);
        void CopyBufferRegion(GPUResource& Dest, size_t DestOffset, GPUResource& Src, size_t SrcOffset, size_t NumBytes);
        void CopySubresource(GPUResource& Dest, UINT DestSubIndex, GPUResource& Src, UINT SrcSubIndex);
        void CopyCounter(GPUResource& Dest, size_t DestOffset, NestedBuffer& Src);
        void ResetCounter(NestedBuffer& Buf, uint32_t Value = 0);

        LinearBuffer ReserveUploadMemory(size_t SizeInBytes)
        {
            return m_CPULinearAllocator.Allocate(SizeInBytes);
        }

        static void InitializeTexture(GPUResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
        static void InitializeBuffer(GPUResource& Dest, const void* Data, size_t NumBytes, size_t Offset = 0);
        static void InitializeTextureArraySlice(GPUResource& Dest, UINT SliceIndex, GPUResource& Src);
        static void ReadbackTexture2D(GPUResource& ReadbackBuffer, PixelBuffer& SrcBuffer);

        void WriteBuffer(GPUResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes);
        void FillBuffer(GPUResource& Dest, size_t DestOffset, float Value, size_t NumBytes);

        void TransitionResource(GPUResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
        void BeginResourceTransition(GPUResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
        void InsertUAVBarrier(GPUResource& Resource, bool FlushImmediate = false);
        void InsertAliasBarrier(GPUResource& Before, GPUResource& After, bool FlushImmediate = false);
        inline void FlushResourceBarriers(void);

        void InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, UINT QueryIdx);
        void ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, UINT NumQueries);
        void PIXBeginEvent(const wchar_t* label);
        void PIXEndEvent(void);
        void PIXSetMarker(const wchar_t* label);

        void SetPipelineState(const PSO& PSO);
        void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
        void SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);

        // Not used.
        void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

    protected:
        void bindDescriptorHeaps(void);

    protected:
        CommandQueueManager* m_owningManager;
        ID3D12GraphicsCommandList* m_commandList;
        ID3D12CommandAllocator* m_currentCommandAllocator;

        ID3D12RootSignature* m_pCurrentGraphicsRootSignature;
        ID3D12PipelineState* m_CurPipelineState;
        ID3D12RootSignature* m_pCurrentComputeRootSignature;

        custom::DynamicDescriptorHeap m_DynamicViewDescriptorHeap;        // For HEAP_TYPE_CBV_SRV_UAV
        custom::DynamicDescriptorHeap m_DynamicSamplerDescriptorHeap;     // For HEAP_TYPE_SAMPLER

        D3D12_RESOURCE_BARRIER m_resourceBarriers[16];
        size_t m_numStandByBarriers;

        ID3D12DescriptorHeap* m_pCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        LinearAllocator m_CPULinearAllocator;
        LinearAllocator m_GPULinearAllocator;

        std::wstring m_ID;
        void setName(const std::wstring& ID) { m_ID = ID; }

        D3D12_COMMAND_LIST_TYPE m_type;
    };
}