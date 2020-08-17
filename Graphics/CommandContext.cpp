#pragma once
#include "stdafx.h"
#include "Graphics.h"
#include "CommandQueue.h"
#include "CommandQueueManager.h"
#include "CommandContextManager.h"
#include "CommandContext.h"
#include "PixelBuffer.h"
#include "UAVBuffer.h"
#include "DynamicDescriptorHeap.h"

namespace custom
{
    CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE Type) :
        m_type(Type),
        m_DynamicViewDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        m_DynamicSamplerDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
        m_CPULinearAllocator(LinearAllocatorType::CPU_WRITEABLE),
        m_GPULinearAllocator(LinearAllocatorType::GPU_WRITEABLE)
    {
        m_owningManager = nullptr;
        m_commandList = nullptr;
        m_currentCommandAllocator = nullptr;
        ZeroMemory(m_pCurrentDescriptorHeaps, sizeof(m_pCurrentDescriptorHeaps));

        m_pCurrentGraphicsRootSignature = nullptr;
        m_CurPipelineState = nullptr;
        m_pCurrentComputeRootSignature = nullptr;
        m_numStandByBarriers = 0;
    }

    CommandContext::~CommandContext(void)
    {
        if (m_commandList != nullptr)
            m_commandList->Release();
    }

    void CommandContext::Initialize(void)
    {
        device::g_commandQueueManager.CreateNewCommandList(&m_commandList, &m_currentCommandAllocator, m_type);
    }

    CommandContext& CommandContext::Begin(const std::wstring ID)
    {
        CommandContext* NewContext = device::g_commandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
        NewContext->setName(ID);
//        if (ID.length() > 0)
//             EngineProfiling::BeginBlock(ID, NewContext);
        return *NewContext;
    }

    uint64_t CommandContext::Finish(bool WaitForCompletion)
    {
        ASSERT(m_type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

        FlushResourceBarriers();

//        if (m_ID.length() > 0)
//            EngineProfiling::EndBlock(this);

        ASSERT(m_currentCommandAllocator != nullptr);

        CommandQueue& Queue = device::g_commandQueueManager.GetQueue(m_type);

        uint64_t FenceValue = Queue.executeCommandList(m_commandList);
        Queue.discardAllocator(FenceValue, m_currentCommandAllocator);
        m_currentCommandAllocator = nullptr;

        m_CPULinearAllocator.CleanupUsedPages(FenceValue);
        m_GPULinearAllocator.CleanupUsedPages(FenceValue);
        m_DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
        m_DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

        if (WaitForCompletion)
        {
            device::g_commandQueueManager.WaitForFence(FenceValue);
        }

        device::g_commandContextManager.FreeContext(this);

        return FenceValue;
    }

    void CommandContext::Reset(void)
    {
        // We only call Reset() on previously freed contexts.  The command list persists, but we must
        // request a new allocator.
        ASSERT(m_commandList != nullptr && m_currentCommandAllocator == nullptr);
        m_currentCommandAllocator = device::g_commandQueueManager.GetQueue(m_type).requestAllocator();
        m_commandList->Reset(m_currentCommandAllocator, nullptr);

        m_pCurrentGraphicsRootSignature = nullptr;
        m_CurPipelineState = nullptr;
        m_pCurrentComputeRootSignature = nullptr;
        m_numStandByBarriers = 0;

        bindDescriptorHeaps();
    }

    void CommandContext::FlushResourceBarriers(void)
    {
        if (m_numStandByBarriers > 0)
        {
            m_commandList->ResourceBarrier(m_numStandByBarriers, m_resourceBarriers);
            m_numStandByBarriers = 0;
        }
    }

    void CommandContext::DestroyAllContexts(void)
    {
        LinearAllocator::DestroyAll();
        DynamicDescriptorHeap::DestroyAll();
        device::g_commandContextManager.DestroyAllContexts();
    }

    uint64_t CommandContext::Flush(bool WaitForCompletion)
    {
        FlushResourceBarriers();

        ASSERT(m_currentCommandAllocator != nullptr);

        uint64_t FenceValue = device::g_commandQueueManager.GetQueue(m_type).executeCommandList(m_commandList);

		if (WaitForCompletion)
		{
			device::g_commandQueueManager.WaitForFence(FenceValue);
		}

        //
        // Reset the command list and restore previous state
        //

        m_commandList->Reset(m_currentCommandAllocator, nullptr);

        if (m_pCurrentGraphicsRootSignature)
        {
            m_commandList->SetGraphicsRootSignature(m_pCurrentGraphicsRootSignature);
        }
        if (m_pCurrentComputeRootSignature)
        {
            m_commandList->SetComputeRootSignature(m_pCurrentComputeRootSignature);
        }
        if (m_CurPipelineState)
        {
            m_commandList->SetPipelineState(m_CurPipelineState);
        }

        bindDescriptorHeaps();

        return FenceValue;
    }

    void CommandContext::BeginResourceTransition(GPUResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
    {
        // If it's already transitioning, finish that transition
        if (Resource.m_transitionState != (D3D12_RESOURCE_STATES)-1)
            TransitionResource(Resource, Resource.m_transitionState);

        D3D12_RESOURCE_STATES OldState = Resource.m_currentState;

        if (OldState != NewState)
        {
            ASSERT(m_numStandByBarriers < 16, "Exceeded arbitrary limit on buffered barriers");
            D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarriers[m_numStandByBarriers++];

            BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            BarrierDesc.Transition.pResource = Resource.GetResource();
            BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            BarrierDesc.Transition.StateBefore = OldState;
            BarrierDesc.Transition.StateAfter = NewState;

            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

            Resource.m_transitionState = NewState;
        }

        if (FlushImmediate || (16 <= m_numStandByBarriers))
        {
            FlushResourceBarriers();
        }
    }
    
    void CommandContext::TransitionResource(GPUResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
    {
        D3D12_RESOURCE_STATES OldState = Resource.m_currentState;

        if (m_type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            // 0xc48 = RESOURCE_STATE UNORDERED_ACCESS | PIXEL_SHADER_RESOURCE | COPY | DEST_SOURCE

            ASSERT((OldState & 0xc48) == OldState);
            ASSERT((NewState & 0xc48) == NewState);
        }

        if (OldState != NewState)
        {
            ASSERT(m_numStandByBarriers < 16, "Exceeded arbitrary limit on buffered barriers");
            D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarriers[m_numStandByBarriers++];

            BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            BarrierDesc.Transition.pResource = Resource.GetResource();
            BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            BarrierDesc.Transition.StateBefore = OldState;
            BarrierDesc.Transition.StateAfter = NewState;

            // Check to see if we already started the transition
            if (NewState == Resource.m_transitionState)
            {
                BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
                Resource.m_transitionState = (D3D12_RESOURCE_STATES)-1;
            }
            else
            {
                BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            }

            Resource.m_currentState = NewState;
        }
		else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			InsertUAVBarrier(Resource, FlushImmediate);
		}

		if (FlushImmediate || (16 <= m_numStandByBarriers))
		{
			FlushResourceBarriers();
		}
    }

    void CommandContext::InsertUAVBarrier(GPUResource& Resource, bool FlushImmediate)
    {
        ASSERT(m_numStandByBarriers < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarriers[m_numStandByBarriers++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        BarrierDesc.UAV.pResource = Resource.GetResource();

		if (FlushImmediate)
		{
			FlushResourceBarriers();
		}
    }

    void CommandContext::InsertAliasBarrier(GPUResource& Before, GPUResource& After, bool FlushImmediate)
    {
        ASSERT(m_numStandByBarriers < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarriers[m_numStandByBarriers++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        BarrierDesc.Aliasing.pResourceBefore = Before.GetResource();
        BarrierDesc.Aliasing.pResourceAfter = After.GetResource();

		if (FlushImmediate)
		{
			FlushResourceBarriers();
		}
    }

    void CommandContext::WriteBuffer(GPUResource& Dest, size_t DestOffset, const void* BufferData, size_t NumBytes)
    {
        ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16));
        LinearBuffer temp = m_CPULinearAllocator.Allocate(NumBytes, 512);
        SIMDMemCopy(temp.pData, BufferData, HashInternal::DivideByMultiple(NumBytes, 16));
        CopyBufferRegion(Dest, DestOffset, temp.buffer, temp.offset, NumBytes);
    }

    void CommandContext::FillBuffer(GPUResource& Dest, size_t DestOffset, float Value, size_t NumBytes)
    {
        LinearBuffer temp = m_CPULinearAllocator.Allocate(NumBytes, 512);
        __m128 VectorValue = _mm_set1_ps(Value);
        SIMDMemFill(temp.pData, VectorValue, HashInternal::DivideByMultiple(NumBytes, 16));
        CopyBufferRegion(Dest, DestOffset, temp.buffer, temp.offset, NumBytes);
    }

    void CommandContext::InitializeTexture(GPUResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
    {
        UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

        CommandContext& InitContext = CommandContext::Begin();

        // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
        LinearBuffer mem = InitContext.ReserveUploadMemory(uploadBufferSize);
        UpdateSubresources(InitContext.m_commandList, Dest.GetResource(), mem.buffer.GetResource(), 0, 0, NumSubresources, SubData);
        InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

        // Execute the command list and wait for it to finish so we can release the upload buffer
        InitContext.Finish(true);
    }

    void CommandContext::InitializeTextureArraySlice(GPUResource& Dest, UINT SliceIndex, GPUResource& Src)
    {
        CommandContext& Context = CommandContext::Begin();

        Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        Context.FlushResourceBarriers();

        const D3D12_RESOURCE_DESC& DestDesc = Dest.GetResource()->GetDesc();
        const D3D12_RESOURCE_DESC& SrcDesc = Src.GetResource()->GetDesc();

        ASSERT
        (
            SliceIndex < DestDesc.DepthOrArraySize&&
            SrcDesc.DepthOrArraySize == 1 &&
            DestDesc.Width == SrcDesc.Width &&
            DestDesc.Height == SrcDesc.Height &&
            DestDesc.MipLevels <= SrcDesc.MipLevels
        );

        UINT SubResourceIndex = SliceIndex * DestDesc.MipLevels;

        for (UINT MipLevel = 0; MipLevel < DestDesc.MipLevels; ++MipLevel)
        {
            D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
            {
                Dest.GetResource(),
                D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                SubResourceIndex + MipLevel
            };

            D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
            {
                Src.GetResource(),
                D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                MipLevel
            };

            Context.m_commandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
        }

        Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);
        Context.Finish(true);
    }

    void CommandContext::CopyBuffer(GPUResource& Dest, GPUResource& Src)
    {
        TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        m_commandList->CopyResource(Dest.GetResource(), Src.GetResource());
    }

    void CommandContext::CopyBufferRegion
    (
        GPUResource& Dest, size_t DestOffset, GPUResource& Src, 
        size_t SrcOffset, size_t NumBytes
    )
    {
        TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        //TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        m_commandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetResource(), SrcOffset, NumBytes);
    }

    void CommandContext::CopySubresource(GPUResource& Dest, UINT DestSubIndex, GPUResource& Src, UINT SrcSubIndex)
    {
        FlushResourceBarriers();

        D3D12_TEXTURE_COPY_LOCATION DestLocation =
        {
            Dest.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            DestSubIndex
        };

        D3D12_TEXTURE_COPY_LOCATION SrcLocation =
        {
            Src.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            SrcSubIndex
        };

        m_commandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
    }

    void CommandContext::CopyCounter(GPUResource& Dest, size_t DestOffset, StructuredBuffer& Src)
    {
        TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
        TransitionResource(Src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
        FlushResourceBarriers();
        m_commandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetCounterBuffer().GetResource(), 0, 4);
    }

    inline void CommandContext::ResetCounter(StructuredBuffer& Buf, uint32_t Value)
    {
        FillBuffer(Buf.GetCounterBuffer(), 0, Value, sizeof(uint32_t));
        TransitionResource(Buf.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    void CommandContext::ReadbackTexture2D(GPUResource& ReadbackBuffer, PixelBuffer& SrcBuffer)
    {
        // The footprint may depend on the device of the resource, but we assume there is only one device.
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
        device::g_pDevice->GetCopyableFootprints(&SrcBuffer.GetResource()->GetDesc(), 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);
        
        // This very short command list only issues one API call and will be synchronized so we can immediately read
        // the buffer contents.
        CommandContext& Context = CommandContext::Begin(L"Copy texture to memory");

        Context.TransitionResource(SrcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

        Context.m_commandList->CopyTextureRegion
        (
            &CD3DX12_TEXTURE_COPY_LOCATION(ReadbackBuffer.GetResource(), PlacedFootprint), 0, 0, 0,
            &CD3DX12_TEXTURE_COPY_LOCATION(SrcBuffer.GetResource(), 0), nullptr
        );

        Context.Finish(true);
    }

    void CommandContext::InitializeBuffer(GPUResource& Dest, const void* BufferData, size_t NumBytes, size_t Offset /* = 0*/)
    {
        CommandContext& InitContext = CommandContext::Begin();

        LinearBuffer mem = InitContext.ReserveUploadMemory(NumBytes);
        SIMDMemCopy(mem.pData, BufferData, HashInternal::DivideByMultiple(NumBytes, 16));

        // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
        InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
        InitContext.m_commandList->CopyBufferRegion(Dest.GetResource(), Offset, mem.buffer.GetResource(), 0, NumBytes);
        InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

        // Execute the command list and wait for it to finish so we can release the upload buffer
        InitContext.Finish(true);
    }

    void CommandContext::SetPipelineState(const PSO& PSO)
    {
        ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
		if (PipelineState == m_CurPipelineState)
		{
			return;
		}

        m_commandList->SetPipelineState(PipelineState);
        m_CurPipelineState = PipelineState;
    }

    void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr)
    {
        if (m_pCurrentDescriptorHeaps[Type] != HeapPtr)
        {
            m_pCurrentDescriptorHeaps[Type] = HeapPtr;
            bindDescriptorHeaps();
        }
    }

    void CommandContext::SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[])
    {
        bool bAnyChanged = false;

        for (size_t i = 0; i < HeapCount; ++i)
        {
            if (m_pCurrentDescriptorHeaps[Type[i]] != HeapPtrs[i])
            {
                m_pCurrentDescriptorHeaps[Type[i]] = HeapPtrs[i];
                bAnyChanged = true;
            }
        }

		if (bAnyChanged)
		{
			bindDescriptorHeaps();
		}
    }

    void CommandContext::InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, UINT QueryIdx)
    {
        // For Gpu Profiling.
        m_commandList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
    }

    void CommandContext::ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, UINT NumQueries)
    {
        // For Gpu Profiling.
        m_commandList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
    }

    void CommandContext::PIXBeginEvent(const wchar_t* label)
    {
#if defined(_DEBUG) | !defined(RELEASE)
        ::PIXBeginEvent(m_commandList, 0, label);
#endif
    }

    void CommandContext::PIXEndEvent(void)
    {
#if defined(_DEBUG) | !defined(RELEASE)
        ::PIXEndEvent(m_commandList);
#endif
    }

    void CommandContext::PIXSetMarker(const wchar_t* label)
    {
#if defined(_DEBUG) | !defined(RELEASE)
        ::PIXSetMarker(m_commandList, 0, label);
#endif
    }

    void CommandContext::bindDescriptorHeaps()
    {
        UINT NonNullHeaps = 0;
        ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        for (size_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            ID3D12DescriptorHeap* HeapIter = m_pCurrentDescriptorHeaps[i];
			// if (HeapIter != nullptr)
			// {
			// 	HeapsToBind[NonNullHeaps++] = HeapIter;
			// }
            HeapsToBind[NonNullHeaps] = HeapIter;
            NonNullHeaps += ((size_t)HeapIter & 1);
        }

		if (0 < NonNullHeaps)
		{
			m_commandList->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
		}
    }

    // Not used.
    inline void CommandContext::SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
    {
        m_commandList->SetPredication(Buffer, BufferOffset, Op);
    }


}
