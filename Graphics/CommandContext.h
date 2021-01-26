#pragma once
#include "GPUResource.h"
#include "UAVBuffer.h"
#include "LinearAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "CommandSignature.h"

#include "Color.h"
#include "Vector.h"
#include "Matrix4.h"
#include "ShaderConstantsTypeDefinitions.h"

class CommandContextManager;
class CommandQueueManager;
class PixelBuffer;
class DepthBuffer;
class ColorBuffer;
class IBaseCamera;
class Camera;
class ShadowCamera;
class MainLight;

#define CLASS_NO_COPY(className)                         \
    private :                                            \
        className(const className&) = delete;            \
        className(className&&) = delete;                 \
        className& operator=(const className&) = delete; \
        className& operator=(className&&) = delete;

namespace custom
{
    class RootSignature;
    class GpuResource;
    class GraphicsContext;
    class ComputeContext;
    class PSO;

    class CommandContext
    {
        friend CommandContextManager;
        // CLASS_NO_COPY(CommandContext);
    private:
        CommandContext(D3D12_COMMAND_LIST_TYPE Type);
        // CommandContext(const CommandContext&);
        void Reset(std::wstring ID = L"");

    public:
        ~CommandContext();

        static void DestroyAllContexts();

        static CommandContext& Begin(const std::wstring& ID = L"");

        // Flush existing commands to the GPU but keep the context alive
        uint64_t Flush(bool WaitForCompletion = false);

        // Flush existing commands and release the current context
        uint64_t Finish(bool WaitForCompletion = false, bool RecordForSwapChainFence = false);

        // Prepare to render by reserving a command list and command allocator
        void Initialize(const std::wstring& ID);

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
        void CopyCounter(GPUResource& Dest, size_t DestOffset, StructuredBuffer& Src);
        void ResetCounter(StructuredBuffer& Buf, float Value = 0.0f);

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
        void PIXEndEvent();
        void PIXSetMarker(const wchar_t* label);

        void SetPipelineState(const PSO& PSO);
        void SetPipelineStateByPtr(ID3D12PipelineState* pPSO);
        void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
        void SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);

        // Will not be used.
        void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

        void SetModelToProjection(const Math::Matrix4& _ViewProjMatrix);
        void SetModelToProjectionByCamera(IBaseCamera& _IBaseCamera); // Camera Or ShadowCamera

        void SetModelToShadow(const Math::Matrix4& _ShadowMatrix);
        void SetModelToShadowByShadowCamera(ShadowCamera& _ShadowCamera);

        void SetMainCamera(Camera& _Camera);

        Camera* GetpMainCamera();
        VSConstants GetVSConstants();
        PSConstants GetPSConstants();

        void SetMainLightDirection(Math::Vector3 MainLightDirection);
        void SetMainLightColor(Math::Vector3 Color, float Intensity);
        void SetAmbientLightColor(Math::Vector3 Color, float Intensity);
        void SetShadowTexelSize(float TexelSize);
        void SetTileDimension(uint32_t MainColorBufferWidth, uint32_t MainColorBufferHeight, uint32_t LightWorkGroupSize);
        void SetSpecificLightIndex(uint32_t FirstConeLightIndex, uint32_t FirstConeShadowedLightIndex);
        void SetPSConstants(MainLight& _MainLight);
        void SetSync();

    protected:
        void bindDescriptorHeaps();

    protected:
        CommandQueueManager*       m_owningManager;
        ID3D12GraphicsCommandList* m_commandList;
        ID3D12CommandAllocator*    m_currentCommandAllocator;

        ID3D12PipelineState* m_CurPipelineState;
        ID3D12RootSignature* m_pCurrentGraphicsRootSignature;
        ID3D12RootSignature* m_pCurrentComputeRootSignature;

        DynamicDescriptorHeap m_DynamicViewDescriptorHeap;    // For HEAP_TYPE_CBV_SRV_UAV
        DynamicDescriptorHeap m_DynamicSamplerDescriptorHeap; // For HEAP_TYPE_SAMPLER

        UINT                     m_numStandByBarriers;
        D3D12_RESOURCE_BARRIER   m_resourceBarriers[16];
        D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;

        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT     m_Rect;

        ID3D12DescriptorHeap* m_pCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        Camera*       m_pMainCamera;
        ShadowCamera* m_pMainLightShadowCamera;

        VSConstants m_VSConstants;
        PSConstants m_PSConstants;

        LinearAllocator m_CPULinearAllocator;
        LinearAllocator m_GPULinearAllocator;

        std::wstring m_ID;
        void setName(const std::wstring& ID) { m_ID = ID; }

        D3D12_COMMAND_LIST_TYPE m_type;
    };

    class CommandSignature;

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
        void SetOnlyDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(0, nullptr, DSV); }

        void SetViewport(const D3D12_VIEWPORT& Viewport);
        void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
        void SetScissor(const D3D12_RECT& Rect);
        void SetScissor(LONG left, LONG top, LONG right, LONG bottom);
        void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
        void SetViewportAndScissor(LONG x, LONG y, LONG w, LONG h);
        void SetViewportAndScissor(PixelBuffer& TargetBuffer);
        void SetStencilRef(UINT StencilRef) { m_commandList->OMSetStencilRef(StencilRef); }
        void SetBlendFactor(custom::Color BlendFactor) { m_commandList->OMSetBlendFactor(BlendFactor.GetPtr()); }
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants) 
        { 
            m_commandList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0); 
        }
        void SetConstants(UINT RootIndex, float _32BitParameter1, float _32BitParameter2);
        void SetConstants(UINT RootIndex, float _32BitParameter0, float _32BitParameter1, float _32BitParameter2);
        void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV) 
        { 
            m_commandList->SetGraphicsRootConstantBufferView(RootIndex, CBV); 
        }
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

        void SetVSConstantsBuffer(UINT RootIndex);
        void SetPSConstantsBuffer(UINT RootIndex);

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

/*                                      device::g_commandQueueManager
CommandContex 成式 CommandQueueManager*       m_owningManager 成式 ID3D12Device* pDevice == device::g_pDevice
              弛                                             戌式 custom::CommandQueue CommandQueue[3] { graphics, Compute, Copy }
              弛                                                               戍式 ID3D12Fence*         m_pFence    ,   FenceValue
              弛                                                               戍式 ID3D12CommandQueue*  m_commandQueue
              弛                                                               戌式 CommandAllocatorPool m_allocatorPool
              戍式 ID3D12GraphicsCommandList* m_commandList                                                      ∪
              弛                                   ^                                                            弛
              弛::CommandQueueManager >式式式式式式式式式式式式戎                                                            弛
              弛                                                                                                弛
              戍式 ID3D12CommandAllocator*    m_currentCommandAllocator <式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式戎
              弛
              弛
              弛
              戌式 UINT                       m_numStandByBarriers


*/