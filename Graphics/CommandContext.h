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

        ID3D12GraphicsCommandList* GetCommandList() {
            return m_CommandList;
        }

        void CopyBuffer(GPUResource& Dest, GPUResource& Src);
        void CopyBufferRegion(GPUResource& Dest, size_t DestOffset, GPUResource& Src, size_t SrcOffset, size_t NumBytes);
        void CopySubresource(GPUResource& Dest, UINT DestSubIndex, GPUResource& Src, UINT SrcSubIndex);
        void CopyCounter(GPUResource& Dest, size_t DestOffset, NestedBuffer& Src);
        void ResetCounter(NestedBuffer& Buf, uint32_t Value = 0);

        LinearBuffer ReserveUploadMemory(size_t SizeInBytes)
        {
            return m_cpuLinearAllocator.Allocate(SizeInBytes);
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

        void InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx);
        void ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries);
        void PIXBeginEvent(const wchar_t* label);
        void PIXEndEvent(void);
        void PIXSetMarker(const wchar_t* label);

        void SetPipelineState(const PSO& PSO);
        void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
        void SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);

        void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

    protected:

        void BindDescriptorHeaps(void);

        CommandQueueManager* m_OwningManager;
        ID3D12GraphicsCommandList* m_CommandList;
        ID3D12CommandAllocator* m_CurrentAllocator;

        ID3D12RootSignature* m_CurGraphicsRootSignature;
        ID3D12PipelineState* m_CurPipelineState;
        ID3D12RootSignature* m_CurComputeRootSignature;

        DynamicDescriptorHeap m_DynamicViewDescriptorHeap;        // HEAP_TYPE_CBV_SRV_UAV
        DynamicDescriptorHeap m_DynamicSamplerDescriptorHeap;    // HEAP_TYPE_SAMPLER

        D3D12_RESOURCE_BARRIER m_ResourceBarrierBuffer[16];
        UINT m_NumBarriersToFlush;

        ID3D12DescriptorHeap* m_CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        LinearAllocator m_cpuLinearAllocator;
        LinearAllocator m_gpuLinearAllocator;

        std::wstring m_ID;
        void SetID(const std::wstring& ID) { m_ID = ID; }

        D3D12_COMMAND_LIST_TYPE m_type;
    };
}