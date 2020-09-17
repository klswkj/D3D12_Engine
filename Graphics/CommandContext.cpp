#pragma once
#include "stdafx.h"
#include "Graphics.h"
#include "CommandQueue.h"
#include "CommandQueueManager.h"
#include "CommandContextManager.h"
#include "CommandContext.h"
#include "CommandSignature.h"
#include "DepthBuffer.h"
#include "PixelBuffer.h"
#include "GPUResource.h"
#include "UAVBuffer.h"
#include "BufferManager.h"
#include "DynamicDescriptorHeap.h"
#include "PSO.h"
#include "RootSignature.h"
#include "Color.h"
#include "GPUResource.h"
#include "UAVBuffer.h"
#include "LinearAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "CommandSignature.h"

#include "IBaseCamera.h"
#include "Camera.h"
#include "ShadowCamera.h"

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

	void CommandContext::FlushResourceBarriers()
	{
		if (0 < m_numStandByBarriers)
		{
			m_commandList->ResourceBarrier(m_numStandByBarriers, m_resourceBarriers);
			m_numStandByBarriers = 0;
		}
	}

	void CommandContext::DestroyAllContexts()
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

		// Reset the command list and restore previous state

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
		{
			TransitionResource(Resource, Resource.m_transitionState);
		}

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

	void CommandContext::ResetCounter(StructuredBuffer& Buf, uint32_t Value)
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

	void CommandContext::SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
	{
		m_commandList->SetPredication(Buffer, BufferOffset, Op);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	

	void CommandContext::SetModelToProjection(Math::Matrix4& _ViewProjMatrix)
	{
		m_VSConstants.modelToProjection = _ViewProjMatrix;
	}
	void CommandContext::SetModelToProjectionByCamera(IBaseCamera& _IBaseCamera)
	{
		m_VSConstants.modelToProjection = _IBaseCamera.GetProjMatrix();
		m_VSConstants.viewerPos = _IBaseCamera.GetPosition();
	}
	void CommandContext::SetModelToShadow(Math::Matrix4& _ShadowMatrix)
	{
		m_VSConstants.modelToShadow = _ShadowMatrix;
	}
	void CommandContext::SetModelToShadowByShadowCamera(ShadowCamera& _ShadowCamera)
	{
		m_pMainLightShadowCamera = &_ShadowCamera;
		m_VSConstants.modelToShadow = m_pMainLightShadowCamera->GetShadowMatrix();
	}
	void CommandContext::SetMainCamera(Camera& _Camera)
	{
		m_pMainCamera = &_Camera;
		m_VSConstants.viewerPos = _Camera.GetPosition();
	}

	Camera* CommandContext::GetpMainCamera()
	{
		ASSERT(m_pMainCamera != nullptr);

		return m_pMainCamera;
	}

	void CommandContext::SetMainLightDirection(Math::Vector3 MainLightDirection)
	{
		m_PSConstants.sunDirection = MainLightDirection;
	}
	void CommandContext::SetMainLightColor(Math::Vector3 Color, float Intensity)
	{
		m_PSConstants.sunLight = Color * Intensity;
	}
	void CommandContext::SetAmbientLightColor(Math::Vector3 Color, float Intensity)
	{
		m_PSConstants.ambientLight = Color * Intensity;
	}
	void CommandContext::SetShadowTexelSize(float TexelSize)
	{
		if (1.0f < TexelSize)
		{
			TexelSize = 1.0f / TexelSize;
		}

		m_PSConstants.ShadowTexelSize[0] = TexelSize;
	}

	void CommandContext::SetTileDimension(float MainColorBufferWidth, float MainColorBufferHeight, uint32_t LightWorkGroupSize)
	{
		ASSERT(1.0f < LightWorkGroupSize);

		m_PSConstants.InvTileDim[0] = 1.0f / LightWorkGroupSize;
		m_PSConstants.InvTileDim[1] = m_PSConstants.InvTileDim[0];

		m_PSConstants.TileCount[0] = HashInternal::DivideByMultiple(MainColorBufferWidth, LightWorkGroupSize);
		m_PSConstants.TileCount[0] = HashInternal::DivideByMultiple(MainColorBufferHeight, LightWorkGroupSize);
	}

	void CommandContext::SetSpecificLightIndex(uint32_t FirstConeLightIndex, uint32_t FirstConeShadowedLightIndex)
	{
		m_PSConstants.FirstLightIndex[0] = FirstConeLightIndex;
		m_PSConstants.FirstLightIndex[1] = FirstConeShadowedLightIndex;
	}

	void CommandContext::SetSync()
	{
		device::g_commandContextManager.m_VSConstants = m_VSConstants;
		device::g_commandContextManager.m_PSConstants = m_PSConstants;
		device::g_commandContextManager.m_pCamera = m_pMainCamera;
		device::g_commandContextManager.m_pShadowCamera = m_pMainLightShadowCamera;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// TODO : Eliminate Overloading.
	void GraphicsContext::SetRootSignature(const RootSignature& RootSig)
	{
		if (RootSig.GetSignature() == m_pCurrentGraphicsRootSignature)
		{
			return;
		}

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

	void GraphicsContext::SetScissor(LONG left, LONG top, LONG right, LONG bottom)
	{
		SetScissor(CD3DX12_RECT(left, top, right, bottom));
	}

	void GraphicsContext::SetViewportAndScissor(LONG x, LONG y, LONG w, LONG h)
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

	void GraphicsContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
	{
		ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16));
		LinearBuffer cb = m_CPULinearAllocator.Allocate(BufferSize);
		//SIMDMemCopy(cb.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
		memcpy(cb.pData, BufferData, BufferSize);
		m_commandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GPUAddress);
	}

	void GraphicsContext::SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VertexData)
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

	void GraphicsContext::SetDynamicIB(size_t IndexCount, const uint16_t* IndexData)
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

	void GraphicsContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
	{
		ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16));
		LinearBuffer cb = m_CPULinearAllocator.Allocate(BufferSize);
		SIMDMemCopy(cb.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
		m_commandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GPUAddress);
	}

	void GraphicsContext::SetBufferSRV(UINT RootIndex, const UAVBuffer& SRV, UINT64 Offset)
	{
		ASSERT((SRV.m_currentState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
		m_commandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
	}

	void GraphicsContext::SetBufferUAV(UINT RootIndex, const UAVBuffer& UAV, UINT64 Offset)
	{
		ASSERT((UAV.m_currentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
		m_commandList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
	}

	void GraphicsContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
	{
		SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
	}

	void GraphicsContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		m_DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
	}

	void GraphicsContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
	{
		SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
	}

	void GraphicsContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		m_DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
	}

	void GraphicsContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
	{
		m_commandList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
	}

	void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
	{
		m_commandList->IASetIndexBuffer(&IBView);
	}

	void GraphicsContext::SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
	{
		SetVertexBuffers(Slot, 1, &VBView);
	}

	void GraphicsContext::SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
	{
		m_commandList->IASetVertexBuffers(StartSlot, Count, VBViews);
	}

	void GraphicsContext::SetVSConstantsBuffer(UINT RootIndex)
	{
		SetDynamicConstantBufferView(RootIndex, sizeof(m_VSConstants), &m_VSConstants);
	}
	void GraphicsContext::SetPSConstantsBuffer(UINT RootIndex)
	{
		SetDynamicConstantBufferView(RootIndex, sizeof(m_PSConstants), &m_PSConstants);
	}

	void GraphicsContext::Draw(UINT VertexCount, UINT VertexStartOffset)
	{
		DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
	}

	void GraphicsContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
	{
		DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
	}

	void GraphicsContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
		UINT StartVertexLocation, UINT StartInstanceLocation)
	{
		FlushResourceBarriers();
		m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
		m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
		m_commandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	void GraphicsContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
		INT BaseVertexLocation, UINT StartInstanceLocation)
	{
		FlushResourceBarriers();
		m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
		m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
		m_commandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}

	void GraphicsContext::ExecuteIndirect
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

	void GraphicsContext::DrawIndirect(UAVBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
	{
		ExecuteIndirect(graphics::g_DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
	}
}
