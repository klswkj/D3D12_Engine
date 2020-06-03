#pragma once

namespace custom
{
    class GraphicContext;
    class ComputeContext;

    class CommandContext
    {
        friend ContextManager;
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

        GraphicsContext& GetGraphicsContext() {
            ASSERT(m_Type != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics");
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

        LinearAllocationPage ReserveUploadMemory(size_t SizeInBytes)
        {
            return m_CpuLinearAllocator.Allocate(SizeInBytes);
        }

        static void InitializeTexture(GPUResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
        static void InitializeBuffer(GPUResource& Dest, const void* Data, size_t NumBytes, size_t Offset = 0);
        static void InitializeTextureArraySlice(GPUResource& Dest, UINT SliceIndex, GPUResource& Src);
        static void ReadbackTexture2D(GPUResource& ReadbackBuffer, PixelBuffer& SrcBuffer);

        void WriteBuffer(GPUResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes);
        void FillBuffer(GPUResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes);

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

        CommandListManager* m_OwningManager;
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

        LinearAllocator m_CpuLinearAllocator;
        LinearAllocator m_GpuLinearAllocator;

        std::wstring m_ID;
        void SetID(const std::wstring& ID) { m_ID = ID; }

        D3D12_COMMAND_LIST_TYPE m_Type;
    };

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

        void SetViewport(const D3D12_VIEWPORT& vp);
        void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
        void SetScissor(const D3D12_RECT& rect);
        void SetScissor(UINT left, UINT top, UINT right, UINT bottom);
        void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
        void SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h);
        void SetStencilRef(UINT StencilRef);
        void SetBlendFactor(Color BlendFactor);
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetConstant(UINT RootIndex, DWParam Val, UINT Offset = 0);
        void SetConstants(UINT RootIndex, DWParam X);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
        void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
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
        void ExecuteIndirect(CommandSignature& CommandSig, UAVBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
            uint32_t MaxCommands = 1, UAVBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);

    private:
    };
}