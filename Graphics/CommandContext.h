#pragma once
#include "GPUResource.h"
#include "LinearAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "Color.h"
#include "ShaderConstantsTypeDefinitions.h"

#pragma region FORWARD_DECLARATION

namespace custom
{
    class RootSignature;
    class GpuResource;
    class GraphicsContext;
    class ComputeContext;
    class CopyContext;
    class PSO;
    class CommandSignature;
    class UAVBuffer;
}

class CommandContextManager;
class CommandQueueManager;
class TaskFiber;
class PixelBuffer;
class DepthBuffer;
class ColorBuffer;
class IBaseCamera;
class Camera;
class ShadowCamera;
class MainLight;

#pragma endregion FORWARD_DECLARATION

#pragma region DEBUG_COMMAND_COUNT

#define CLASS_NO_COPY(className)                         \
    private :                                            \
        className(const className&) = delete;            \
        className(className&&) = delete;                 \
        className& operator=(const className&) = delete; \
        className& operator=(className&&) = delete;

#ifndef CHECK_VALID_INDEX
#define CHECK_VALID_COMMAND_INDEX (commandIndex < m_NumCommandPair)
#endif
#ifndef CHECK_VALID_RANGE
#define CHECK_VALID_RANGE (startCommandIndex <= endCommandIndex) && (endCommandIndex < m_NumCommandPair)
#endif

#if defined(_DEBUG)
#define INCRE_DEBUG_DRAW_JOB_COUNT                \
++m_GPUTaskFiberContexts[commandIndex].NumDrawJobs; 
#elif
#define INCRE_DEBUG_DRAW_JOB_COUNT ;
#endif                                             
#if defined(_DEBUG)
#define INCRE_DEBUG_DRAW_JOB_COUNT_I    \
++m_GPUTaskFiberContexts[i].NumDrawJobs; 
#elif
#define INCRE_DEBUG_DRAW_JOB_COUNT_I ;
#endif

#if defined(_DEBUG)
#define INCRE_DEBUG_SET_RS_COUNT                \
++m_GPUTaskFiberContexts[commandIndex].NumSetRSs; 
#elif
#define INCRE_DEBUG_SET_RS_COUNT ;
#endif                                             
#if defined(_DEBUG)
#define INCRE_DEBUG_SET_RS_COUNT_I      \
++m_GPUTaskFiberContexts[i].NumSetRSs; 
#elif
#define INCRE_DEBUG_SET_RS_COUNT_I ;
#endif

#if defined(_DEBUG)
#define INCRE_DEBUG_SET_PSO_COUNT                \
++m_GPUTaskFiberContexts[commandIndex].NumSetPSOs; 
#elif
#define INCRE_DEBUG_SET_PSO_COUNT ;
#endif                                             
#if defined(_DEBUG)
#define INCRE_DEBUG_SET_PSO_COUNT_I      \
++m_GPUTaskFiberContexts[i].NumSetPSOs; 
#elif
#define INCRE_DEBUG_SET_PSO_COUNT_I ;
#endif

#if defined(_DEBUG)
#define INCRE_DEBUG_SET_DESCRIPTOR_HEAPS_COUNT    \
++m_GPUTaskFiberContexts[commandIndex].NumSetDescriptorHeaps; 
#elif
#define INCRE_DEBUG_SET_DESCRIPTOR_HEAPS_COUNT ;
#endif                                             
#if defined(_DEBUG)
#define INCRE_DEBUG_SET_DESCRIPTOR_HEAPS_COUNT_I      \
++m_GPUTaskFiberContexts[i].NumSetDescriptorHeaps; 
#elif
#define INCRE_DEBUG_SET_DESCRIPTOR_HEAPS_COUNT_I ;
#endif

#if defined(_DEBUG)
#define INCRE_DEBUG_SET_ROOT_PARAM_COUNT             \
++m_GPUTaskFiberContexts[commandIndex].NumSetRootParams; 
#elif
#define INCRE_DEBUG_SET_DESCRIPTOR_HEAPS_COUNT ;
#endif                                             
#if defined(_DEBUG)
#define INCRE_DEBUG_SET_ROOT_PARAM_COUNT_I   \
++m_GPUTaskFiberContexts[i].NumSetRootParams; 
#elif
#define INCRE_DEBUG_SET_DESCRIPTOR_HEAPS_COUNT_I ;
#endif

#if defined(_DEBUG)
#define INCRE_DEBUG_SET_RESOURCE_BARRIER_COUNT                \
++m_GPUTaskFiberContexts[commandIndex].NumSetResourceBarriers; 
#elif
#define INCRE_DEBUG_SET_RESOURCE_BARRIER_COUNT ;
#endif                                             
#if defined(_DEBUG)
#define INCRE_DEBUG_SET_RESOURCE_BARRIER_COUNT_I   \
++m_GPUTaskFiberContexts[i].NumSetResourceBarriers; 
#elif
#define INCRE_DEBUG_SET_RESOURCE_BARRIER_COUNT_I ;
#endif

#if defined(_DEBUG)
#define INCRE_DEBUG_ASYNC_THING_COUNT                 \
++m_GPUTaskFiberContexts[commandIndex].NumAsyncThings; 
#elif
#define INCRE_DEBUG_ASYNC_THING_COUNT ;
#endif                                             
#if defined(_DEBUG)
#define INCRE_DEBUG_ASYNC_THING_COUNT_I    \
++m_GPUTaskFiberContexts[i].NumAsyncThings; 
#elif
#define INCRE_DEBUG_ASYNC_THING_COUNT ;
#endif

#if defined(_DEBUG)
#define CHECK_SMALL_COMMAND_LISTS_EXECUTED(x)\
    for (auto& e : m_GPUTaskFiberContexts)   \
    {                                        \
        e.NumDrawJobs = 0u;                  \
        e.NumSetRSs = 0u;                    \
        e.NumSetPSOs = 0u;                   \
        e.NumSetResourceBarriers = 0u;       \
        e.NumSetDescriptorHeaps = 0u;        \
        e.NumSetRootParams = 0u;             \
        e.NumAsyncThings = 0u;               \
    }
#endif

#pragma endregion DEBUG_COMMAND_COUNT

namespace custom
{
    class CommandContext
    {
        friend CommandContextManager;
        friend class DynamicDescriptorHeap;
    private:
        explicit CommandContext(const D3D12_COMMAND_LIST_TYPE type, const uint8_t numCommandsPair);
        void Reset(const D3D12_COMMAND_LIST_TYPE type, const uint8_t numCommandALs);

    public:
        static void BeginFrame();
        static void EndFrame();
        static CommandContext& Begin
        (
            const D3D12_COMMAND_LIST_TYPE type,
            const uint8_t numCommandALs
        );

        static CommandContext& Resume(const D3D12_COMMAND_LIST_TYPE type, uint64_t contextID);
        static void DestroyAllContexts();

    public:
        virtual ~CommandContext();
        // Execute existing commands to the GPU but keep the context alive
        uint64_t ExecuteCommands(const uint8_t lastCommandALIndex, const bool bRootSigFlush, const bool bPSOFlush);
        // Execute existing commands and release the current context
        uint64_t Finish(const bool CPUSideWaitForCompletion = false);

        // Prepare to render by reserving a command list and command allocator
        void Initialize(const D3D12_COMMAND_LIST_TYPE type, const uint8_t NumCommandLists);
        void PauseRecording();

        GraphicsContext& GetGraphicsContext() { ASSERT(m_type == D3D12_COMMAND_LIST_TYPE_DIRECT);  return *reinterpret_cast<custom::GraphicsContext*>(this); }
        ComputeContext&  GetComputeContext()  { ASSERT(m_type == D3D12_COMMAND_LIST_TYPE_COMPUTE); return *reinterpret_cast<custom::ComputeContext*>(this); }
        CopyContext&     GetCopyContext()     { ASSERT(m_type == D3D12_COMMAND_LIST_TYPE_COPY);    return *reinterpret_cast<custom::CopyContext*>(this); }

        inline D3D12_COMMAND_LIST_TYPE GetType()       const { return m_type; }
        inline uint8_t  GetNumStandbyResourceBarrier() const { return m_numStandByBarriers; }
        inline uint64_t GetCommandContextIndex()       const { return m_CommandContextID; }
        inline uint8_t  GetNumCommandLists() const 
        { ASSERT(m_pCommandAllocators.size() == m_pCommandLists.size() && m_pCommandLists.size() == m_NumCommandPair) return m_NumCommandPair; }
        inline ID3D12GraphicsCommandList* GetCommandList(uint8_t commandIndex) const { return m_pCommandLists[commandIndex]; }

        inline void SetResourceTransitionBarrierIndex(const uint8_t index) { ASSERT(index < m_NumCommandPair); m_ResourceBarrierTargetCommandIndex = index; }

        void WaitLastExecuteGPUSide(CommandContext& BaseContext);

        inline LinearBuffer ReserveUploadMemory(size_t SizeInBytes) { return m_CPULinearAllocator.Allocate(SizeInBytes); }

        // 다른 CommandList, Queue에도 영향을 미침. -> 멀티스레드와 같이 멀티엔진으로 쓰면??? 동기화는?
        void InsertUAVBarrier  (GPUResource& targetResource);
        void InsertAliasBarrier(GPUResource& beforeResource, GPUResource& afterResource);

        void TransitionResource     (GPUResource& targetResource, const D3D12_RESOURCE_STATES stateAfter, const UINT subResourceIndex, D3D12_RESOURCE_STATES forcedOldState = (D3D12_RESOURCE_STATES)-1);
        void SplitResourceTransition(GPUResource& targetResource, const D3D12_RESOURCE_STATES statePending, const UINT subResourceIndex);

        void TransitionResources     (GPUResource& targetResource, const D3D12_RESOURCE_STATES stateAfter, UINT subResourceFlag);
        void SplitResourceTransitions(GPUResource& targetResource, const D3D12_RESOURCE_STATES statePending, UINT subResourceFlag);
        
        void SubmitExternalResourceBarriers(const D3D12_RESOURCE_BARRIER externalResourceBarriers[], const UINT numResourceBarriers, const uint8_t commandIndex);
        void SubmitResourceBarriers(uint8_t commandListIndex);

        void InsertTimeStamp  (ID3D12QueryHeap* const pQueryHeap, const UINT QueryIdx, const uint8_t commandIndex) const;
        void ResolveTimeStamps(ID3D12Resource* const pReadbackHeap, ID3D12QueryHeap* const pQueryHeap, const UINT NumQueries, const uint8_t commandIndex) const;
        void PIXBeginEvent   (const wchar_t* const label, const uint8_t commandIndex) const;
        void PIXEndEvent     (const uint8_t commandIndex) const;
        void PIXBeginEventAll(const wchar_t* const label) const;
        void PIXEndEventAll  () const;
        void PIXSetMarker    (const wchar_t* const label, const uint8_t commandIndex) const;
        void PIXSetMarkerAll (const wchar_t* const label) const;

        void SetPipelineState          (const PSO& customPSO, const uint8_t commandIndex);
        void SetPipelineStateRange     (const PSO& customPSO, const uint8_t startCommandIndex, const uint8_t endCommandIndex);
        void SetPipelineStateByPtr     (ID3D12PipelineState* const pPSO, const uint8_t commandIndex);
        void SetPipelineStateByPtrRange(ID3D12PipelineState* const pPSO, const uint8_t startCommandIndex, const uint8_t endCommandIndex);
        void SetPredication            (ID3D12Resource* const Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op, const uint8_t commandIndex);

        void SetModelToProjection          (const Math::Matrix4& _ViewProjMatrix);
        void SetModelToProjectionByCamera  (const IBaseCamera& _IBaseCamera);
        void SetModelToShadow              (const Math::Matrix4& _ShadowMatrix);
        void SetModelToShadowByShadowCamera(ShadowCamera& _ShadowCamera);

        void SetMainCamera        (Camera& _Camera);
        void SetMainLightDirection(const Math::Vector3& MainLightDirection);
        void SetMainLightColor    (const Math::Vector3& Color, const float Intensity);
        void SetAmbientLightColor (const Math::Vector3& Color, const float Intensity);
        void SetShadowTexelSize   (const float TexelSize);
        void SetTileDimension     (const uint32_t MainColorBufferWidth, const uint32_t MainColorBufferHeight, const uint32_t LightWorkGroupSize);
        void SetSpecificLightIndex(const uint32_t FirstConeLightIndex, const uint32_t FirstConeShadowedLightIndex);
        void SetPSConstants       (MainLight& _MainLight);
        void SetSync              ();

        Camera*     GetpMainCamera() const;
        VSConstants GetVSConstants() const;
        PSConstants GetPSConstants() const;

    protected:
        void setCurrentDescriptorHeaps(const uint8_t ALIndex);
        void setDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* const HeapPtr, const uint8_t commandIndex);
        void setDescriptorHeaps(const UINT HeapCount, const D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* const HeapPtrs[], const uint8_t commandIndex);

    protected:
        LIST_ENTRY m_ContextEntry;
        uint64_t   m_bRecordingFlag; // Present Can be used 64th bit this Command itself, others are commandlist's.
        uint64_t   m_LastExecuteFenceValue;
        uint64_t   m_CommandContextID;
        DWORD      m_MainWorkerThreadID;
        CommandQueueManager*                    m_pOwningManager;
        std::vector<ID3D12CommandAllocator*>    m_pCommandAllocators;
        std::vector<ID3D12GraphicsCommandList*> m_pCommandLists;

        // TODO 0 : 굳이 commandList갯수랑 똑같이 늘릴 필요가 없음.
        // 갯수를 최소화하면 할 수록, cacheFlush될 필요가 없음.

        // 잡카운트, RS, PSO, 리소스배리어 set한 수
        struct GPUTaskFiberContext
        {
            GPUTaskFiberContext(CommandContext& BaseContext)
                :
                pCurrentPSO(nullptr),
                pCurrentGraphicsRS(nullptr),
                pCurrentComputeRS(nullptr),
                DynamicViewDescriptorHeaps(BaseContext, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
                DynamicSamplerDescriptorHeaps(BaseContext, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
#ifdef _DEBUG
                ,NumDrawJobs(0u),
                NumSetRSs(0u),
                NumSetPSOs(0u),
                NumSetResourceBarriers(0u),
                NumSetDescriptorHeaps(0u),
                NumSetRootParams(0u),
                NumAsyncThings(0u)
#endif
            {
                ZeroMemory(pCurrentDescriptorHeaps, sizeof(pCurrentDescriptorHeaps));
            }

            ID3D12DescriptorHeap* pCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { nullptr };
            ID3D12PipelineState*  pCurrentPSO        = nullptr;
            ID3D12RootSignature*  pCurrentGraphicsRS = nullptr;
            ID3D12RootSignature*  pCurrentComputeRS  = nullptr;
            DynamicDescriptorHeap DynamicViewDescriptorHeaps;
            DynamicDescriptorHeap DynamicSamplerDescriptorHeaps;
            // uint8_t TargetCommandList;
#ifdef _DEBUG
            uint32_t NumDrawJobs;
            uint32_t NumSetRSs;
            uint32_t NumSetPSOs;
            uint32_t NumSetResourceBarriers;
            uint32_t NumSetDescriptorHeaps;
            uint32_t NumSetRootParams;
            // Clear RT, Clear Depth, Clear UAV etc...
            uint32_t NumAsyncThings;
#endif
        };
        std::vector<GPUTaskFiberContext> m_GPUTaskFiberContexts;

        LinearAllocator m_CPULinearAllocator;
        LinearAllocator m_GPULinearAllocator;

        uint8_t m_ResourceBarrierTargetCommandIndex;
        uint8_t m_EndCommandALIndex;
        uint8_t m_NumCommandPair;
        uint8_t m_numStandByBarriers;
        D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
        D3D12_COMMAND_LIST_TYPE  m_type;
        D3D12_RESOURCE_BARRIER   m_resourceBarriers[16]; // Only MainThread access exclusively.

        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT     m_Rect;

        Camera*       m_pMainCamera;
        ShadowCamera* m_pMainLightShadowCamera;

        VSConstants m_VSConstants;
        PSConstants m_PSConstants;
    };

    class GraphicsContext final : public CommandContext
    {
    public:
		static GraphicsContext& Begin(uint8_t numCommandALs);
		static GraphicsContext& Resume(uint64_t contextID);
		static GraphicsContext& GetRecentContext();

    public:
        void CopySubresource
        (
            GPUResource& dest, UINT destSubIndex,
            GPUResource& src, UINT srcSubIndex,
            const uint8_t commandIndex
        );

    public:
        void ClearUAV(const UAVBuffer& Target, const uint8_t commandIndex);
        void ClearUAV(const ColorBuffer& Target, const uint8_t commandIndex);
        void ClearColor(const ColorBuffer& Target, const uint8_t commandIndex);
        void ClearDepth(const DepthBuffer& Target, const uint8_t commandIndex);
        void ClearStencil(const DepthBuffer& Target, const uint8_t commandIndex);
        void ClearDepthAndStencil(const DepthBuffer& Target, const uint8_t commandIndex);

        void BeginQuery(ID3D12QueryHeap* const QueryHeap, const D3D12_QUERY_TYPE Type, const UINT HeapIndex, const uint8_t commandIndex);
        void EndQuery(ID3D12QueryHeap* const QueryHeap, const D3D12_QUERY_TYPE Type, const UINT HeapIndex, const uint8_t commandIndex);
        void ResolveQueryData(ID3D12QueryHeap* const QueryHeap, const D3D12_QUERY_TYPE Type, const UINT StartIndex, const UINT NumQueries, ID3D12Resource* const DestinationBuffer, const UINT64 DestinationBufferOffset, const uint8_t commandIndex);

        void SetRootSignature(const RootSignature& customRS, const uint8_t commandIndex);
        void SetRootSignatureRange(const RootSignature& customRS, const uint8_t startCommandIndex, const uint8_t endCommandIndex);

        void SetRenderTargets(const UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], const uint8_t commandIndex);
        void SetRenderTargets(const UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], const D3D12_CPU_DESCRIPTOR_HANDLE DSV, const uint8_t commandIndex);
        void SetRenderTargetsRange(const UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], const D3D12_CPU_DESCRIPTOR_HANDLE DSV, const uint8_t startCommandIndex, const uint8_t endCommandIndex);
        void SetRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE RTV, const uint8_t commandIndex) { SetRenderTargets(1, &RTV, commandIndex); }
        void SetRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE RTV, const D3D12_CPU_DESCRIPTOR_HANDLE DSV, const uint8_t commandIndex) { SetRenderTargets(1, &RTV, DSV, commandIndex); }
        void SetOnlyDepthStencil(const D3D12_CPU_DESCRIPTOR_HANDLE DSV, const uint8_t commandIndex) { SetRenderTargets(0, nullptr, DSV, commandIndex); }
        void SetOnlyDepthStencilRange(const D3D12_CPU_DESCRIPTOR_HANDLE DSV, const uint8_t startCommandIndex, const uint8_t endCommandIndex) { SetRenderTargetsRange(0, nullptr, DSV, startCommandIndex, endCommandIndex);}

        void SetViewport(const D3D12_VIEWPORT& Viewport, const uint8_t commandIndex);
        void SetViewport(const FLOAT x, const FLOAT y, const FLOAT w, const FLOAT h, const FLOAT minDepth = 0.0f, const FLOAT maxDepth = 1.0f, const uint8_t commandIndex = -1);
        void SetScissor(const D3D12_RECT& Rect, const uint8_t commandIndex);
        void SetScissor(const LONG left, const LONG top, const LONG right, const LONG bottom, const uint8_t commandIndex);
        void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, const uint8_t commandIndex);
        void SetViewportAndScissorRange(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, const uint8_t startCommandIndex, const uint8_t endCommandIndex);
        void SetViewportAndScissor(const LONG x, const LONG y, const LONG w, const LONG h, const uint8_t commandIndex);
        void SetViewportAndScissor(const PixelBuffer& TargetBuffer, const uint8_t commandIndex);
        void SetViewportAndScissorRange(const PixelBuffer& TargetBuffer, const uint8_t startCommandIndex, const uint8_t endCommandIndex);
        void SetStencilRef(const UINT StencilRef, const uint8_t commandIndex) { m_pCommandLists[commandIndex]->OMSetStencilRef(StencilRef); }
        void SetBlendFactor(custom::Color& BlendFactor, const uint8_t commandIndex) { m_pCommandLists[commandIndex]->OMSetBlendFactor(BlendFactor.GetPtr()); }
        void SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY Topology, bool bForced = false);

        void SetConstantArray(const UINT RootIndex, const UINT NumConstants, const void* const pConstants, const uint8_t commandIndex)
        { 
            m_pCommandLists[commandIndex]->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0); 
        }
        void SetConstants(const UINT RootIndex, float _32BitParameter1, float _32BitParameter2, const uint8_t commandIndex);
        void SetConstants(const UINT RootIndex, float _32BitParameter0, float _32BitParameter1, float _32BitParameter2, const uint8_t commandIndex);
        void SetConstantBuffer(const UINT RootIndex, const D3D12_GPU_VIRTUAL_ADDRESS CBV, const uint8_t commandIndex)
        { 
            m_pCommandLists[commandIndex]->SetGraphicsRootConstantBufferView(RootIndex, CBV); 
        }
        void SetDynamicConstantBufferView(const UINT RootIndex, const size_t BufferSize, const void* const BufferData, const uint8_t commandIndex);
        void SetDynamicConstantBufferViewRange(const UINT RootIndex, const size_t BufferSize, const void* const BufferData, const uint8_t startCommandIndex, const uint8_t endCommandIndex);
        void SetBufferSRV(const UINT RootIndex, const UAVBuffer& SRV, UINT64 Offset, const uint8_t commandIndex);
        void SetBufferUAV(const UINT RootIndex, const UAVBuffer& UAV, UINT64 Offset, const uint8_t commandIndex);
        void SetDescriptorTable(const UINT RootIndex, const  D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle, const uint8_t commandIndex);

        void SetDynamicDescriptor(const UINT RootIndex, const UINT Offset, const D3D12_CPU_DESCRIPTOR_HANDLE Handle, const uint8_t commandIndex);
        void SetDynamicDescriptors(const UINT RootIndex, const UINT Offset, const UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[], const uint8_t commandIndex);
        void SetDynamicDescriptorsRange(const UINT RootIndex, const UINT Offset, const UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[], const uint8_t startCommandIndex, const uint8_t endCommandIndex);
        void SetDynamicSampler(const UINT RootIndex, const UINT Offset, const D3D12_CPU_DESCRIPTOR_HANDLE Handle, const uint8_t commandIndex);
        void SetDynamicSamplers(const UINT RootIndex, const UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[], const uint8_t commandIndex);

        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView, const uint8_t commandIndex);
        void SetVertexBuffer(const UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView, const uint8_t commandIndex);
        void SetVertexBuffers(const UINT StartSlot, const UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[], const uint8_t commandIndex);
        void SetDynamicVB(const UINT Slot, const size_t NumVertices, const size_t VertexStride, const void* const VBData, const uint8_t commandIndex);
        void SetDynamicIB(const size_t IndexCount, const uint16_t* const IBData, const uint8_t commandIndex);
        void SetDynamicSRV(const UINT RootIndex, const size_t BufferSize, const void* const BufferData, const uint8_t commandIndex);
        void SetDynamicSRVRange(const UINT RootIndex, const size_t BufferSize, const void* const BufferData, const uint8_t startCommandIndex, const uint8_t endCommandIndex);

        void SetVSConstantsBuffer(const UINT RootIndex, const uint8_t commandIndex);
        void SetVSConstantsBufferRange(const UINT RootIndex, const uint8_t startCommandIndex, const uint8_t endCommandIndex);
        void SetPSConstantsBuffer(const UINT RootIndex, const uint8_t commandIndex);
        void SetPSConstantsBufferRange(const UINT RootIndex, const uint8_t startCommandIndex, const uint8_t endCommandIndex);

        void Draw(const UINT VertexCount, const UINT VertexStartOffset = 0, const uint8_t commandIndex = -1);
        void DrawIndexed(const UINT IndexCount, const UINT StartIndexLocation = 0, const INT BaseVertexLocation = 0, const uint8_t commandIndex = -1);
        void DrawInstanced(const UINT VertexCountPerInstance, const UINT InstanceCount,
            const UINT StartVertexLocation = 0, const UINT StartInstanceLocation = 0, const uint8_t commandIndex = -1);
        void DrawIndexedInstanced(const UINT IndexCountPerInstance, const UINT InstanceCount, const UINT StartIndexLocation,
            const INT BaseVertexLocation, const UINT StartInstanceLocation, const uint8_t commandIndex = -1);
        void DrawIndirect(const UAVBuffer& ArgumentBuffer, const uint64_t ArgumentBufferOffset = 0, const uint8_t commandIndex = -1);
        void ExecuteIndirect
        (
            const CommandSignature& CommandSig, const UAVBuffer& ArgumentBuffer, const uint64_t ArgumentStartOffset = 0,
            const uint32_t MaxCommands = 1, const UAVBuffer* const CommandCounterBuffer = nullptr, const uint64_t CounterOffset = 0,
            const uint8_t commandIndex = -1
        );
        void ExecuteBundle(ID3D12GraphicsCommandList* const pBundle, const uint8_t commandIndex);

    private:
    };
}