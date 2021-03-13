#pragma once
#include "stdafx.h"
#include "CommandContext.h"
#include "Graphics.h"
#include "CommandQueue.h"
#include "CommandQueueManager.h"
#include "CommandContextManager.h"
#include "CommandSignature.h"
#include "DepthBuffer.h"
#include "UAVBuffer.h"
#include "BufferManager.h"
#include "DynamicDescriptorHeap.h"
#include "PSO.h"
#include "RootSignature.h"
#include "ColorBuffer.h"
#include "LinearAllocator.h"
#include "CommandSignature.h"

#include "IBaseCamera.h"
#include "Camera.h"
#include "ShadowCamera.h"
#include "MainLight.h"

#ifndef VALID_COMPUTE_QUEUE_RESOURCE_STATES
#define VALID_COMPUTE_QUEUE_RESOURCE_STATES          \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS         |\
     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |\
     D3D12_RESOURCE_STATE_COPY_DEST                 |\
     D3D12_RESOURCE_STATE_COPY_SOURCE )
#endif

#ifndef VALID_COPY_QUEUE_RESOURCE_STATES
#define VALID_COPY_QUEUE_RESOURCE_STATES \
       (D3D12_RESOURCE_STATE_COMMON     |\
        D3D12_RESOURCE_STATE_COPY_DEST  |\
        D3D12_RESOURCE_STATE_SOURCE)
#endif

constexpr uint8_t COMMAND_LIMIT = 63u;
constexpr uint8_t NUM_LIMIT_RESOURCE_BARRIER_16 = 16u;

namespace custom
{
#pragma region COMMAND_CONTEXT
	CommandContext::CommandContext(const D3D12_COMMAND_LIST_TYPE Type, const uint8_t numCommandsPair)
		:
		m_type(Type),
		m_LastExecuteFenceValue((uint64_t)Type << 56),
		m_bRecordingFlag(0ull),
		m_StartCommandALIndex(0u),
		m_EndCommandALIndex(0u),
		m_NumCommandPair(numCommandsPair),
		m_MainWorkerThreadID(::GetCurrentThreadId()),
		m_CommandContextID(-1),
		m_Rect({}), m_Viewport({}), m_resourceBarriers(),
		m_CPULinearAllocator(LinearAllocatorType::CPU_WRITEABLE),
		m_GPULinearAllocator(LinearAllocatorType::GPU_WRITEABLE),
		m_pOwningManager(nullptr),
		m_numStandByBarriers(0U),
		m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED),
		m_pMainCamera           (nullptr),
		m_pMainLightShadowCamera(nullptr)
	{
		listHelper::InitializeListHead(&m_ContextEntry);
	}

	CommandContext::~CommandContext()
	{
		for (auto& e : m_pCommandLists)
		{
			CHECK_IUNKOWN_REF_COUNT_DEBUG(SafeRelease(e));
		}
	}

	STATIC void CommandContext::BeginFrame()
	{
		ASSERT(device::g_commandContextManager.NoPausedCommandContext());
	}
	STATIC void CommandContext::EndFrame()
	{
		ASSERT(device::g_commandContextManager.NoPausedCommandContext());
	}
	STATIC CommandContext& CommandContext::Begin(const D3D12_COMMAND_LIST_TYPE type, const uint8_t numCommandALs)
	{
		ASSERT(numCommandALs < COMMAND_LIMIT && CHECK_VALID_TYPE(type));

		CommandContext* NewContext = device::g_commandContextManager.AllocateContext(type, numCommandALs);
		return *NewContext;
	}
	STATIC CommandContext& CommandContext::Resume(const D3D12_COMMAND_LIST_TYPE type, uint64_t contextID)
	{
		custom::CommandContext* ret = nullptr;
		ret = device::g_commandContextManager.GetRecordingContext(type, contextID);
		ASSERT(ret);
		return *ret;
	}
	STATIC void CommandContext::DestroyAllContexts()
	{
		LinearAllocator::DestroyAll();
		DynamicDescriptorHeap::DestroyAll();
		device::g_commandContextManager.DestroyAllCommandContexts();
	}

	void CommandContext::Initialize(const D3D12_COMMAND_LIST_TYPE type, const uint8_t numCommandLists)
	{
		device::g_commandQueueManager.RequestCommandAllocatorLists(m_pCommandAllocators, m_pCommandLists, type, numCommandLists);
	}
	void CommandContext::PauseRecording()
	{
		device::g_commandContextManager.PauseRecording(m_type, this);
	}

	void CommandContext::WaitLastExecuteGPUSide(CommandContext& BaseContext)
	{
		if (m_type == BaseContext.m_type)
		{
			return;
		}

		m_pOwningManager->GetQueue(m_type).WaitFenceValueGPUSide(BaseContext.m_LastExecuteFenceValue);
	}

	uint64_t CommandContext::ExecuteCommands(const uint8_t lastCommandALIndex, const bool bRootSigFlush, const bool bPSOFlush)
	{
		ASSERT(m_pCommandAllocators.size() &&
			CHECK_VALID_TYPE(m_type) &&
			(m_StartCommandALIndex <= lastCommandALIndex) &&
			(lastCommandALIndex < m_NumCommandPair) &&
			(!m_numStandByBarriers));

		CHECK_SMALL_COMMAND_LISTS_EXECUTED(3);

		m_LastExecuteFenceValue = device::g_commandQueueManager.GetQueue(m_type)
			.executeCommandLists(m_pCommandLists, m_StartCommandALIndex, lastCommandALIndex);

		// Reset the command list and restore previous state
		for (uint8_t i = m_StartCommandALIndex; i <= lastCommandALIndex; ++i)
		{
			ASSERT_HR(m_pCommandLists[i]->Reset(m_pCommandAllocators[i], nullptr));

			if (m_GPUTaskFiberContexts[i].pCurrentGraphicsRS && !bRootSigFlush)
			{
				m_pCommandLists[i]->SetGraphicsRootSignature(m_GPUTaskFiberContexts[i].pCurrentGraphicsRS);
				INCRE_DEBUG_SET_RS_COUNT_I;
			}
			if (m_GPUTaskFiberContexts[i].pCurrentComputeRS && !bRootSigFlush)
			{
				m_pCommandLists[i]->SetComputeRootSignature(m_GPUTaskFiberContexts[i].pCurrentComputeRS);
				INCRE_DEBUG_SET_RS_COUNT_I;
			}
			if (m_GPUTaskFiberContexts[i].pCurrentPSO && !bPSOFlush)
			{
				m_pCommandLists[i]->SetPipelineState(m_GPUTaskFiberContexts[i].pCurrentPSO);
				INCRE_DEBUG_SET_PSO_COUNT_I;
			}

			setCurrentDescriptorHeaps(i);
		}
		

		m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

		// m_StartCommandALIndex = (EndALIndex + 1) % m_NumCommandPair;
		// m_bRecordingFlag &= ~(((1 << (EndALIndex - m_StartCommandALIndex + 1)) - 1) << m_StartCommandALIndex);
		// m_bRecordingFlag |= (1ull << 63);

		return m_LastExecuteFenceValue;
	}

	uint64_t CommandContext::Finish(const bool CPUSideWaitForCompletion)
	{
		ASSERT((m_pCommandAllocators.size()) && (!m_numStandByBarriers) && CHECK_VALID_TYPE(m_type));

		CHECK_SMALL_COMMAND_LISTS_EXECUTED(3);

		CommandQueue& Queue = device::g_commandQueueManager.GetQueue(m_type);
		m_LastExecuteFenceValue = Queue.executeCommandLists(m_pCommandLists, m_StartCommandALIndex, m_NumCommandPair - 1);

		m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

		m_CPULinearAllocator.CleanupUsedPages(m_LastExecuteFenceValue);
		m_GPULinearAllocator.CleanupUsedPages(m_LastExecuteFenceValue);

		for (size_t i = m_StartCommandALIndex; i < m_NumCommandPair; ++i)
		{
			m_GPUTaskFiberContexts[i].DynamicViewDescriptorHeaps.CleanupUsedHeaps(m_LastExecuteFenceValue);
			m_GPUTaskFiberContexts[i].DynamicSamplerDescriptorHeaps.CleanupUsedHeaps(m_LastExecuteFenceValue);
		}

		if (CPUSideWaitForCompletion)
		{
			device::g_commandQueueManager.WaitFenceValueCPUSide(m_LastExecuteFenceValue);
		}
#pragma warning(push)
#pragma warning(disable : 26454)
		m_bRecordingFlag &= (uint64_t)((1ull << 63) - 1ull);
#pragma warning(pop)
		device::g_commandContextManager.FreeContext(this);

		return m_LastExecuteFenceValue;
	}

	// Can be Optimized.
	void CommandContext::Reset(D3D12_COMMAND_LIST_TYPE type, uint8_t numCommandALs)
	{
		custom::CommandQueue& Queue = m_pOwningManager->GetQueue(type);
		uint8_t CurrentAllocatorSize = (uint8_t)m_pCommandAllocators.size();

		ASSERT(!m_numStandByBarriers && (CurrentAllocatorSize == m_pCommandLists.size()));

		if (CurrentAllocatorSize < numCommandALs)
		{
			m_pOwningManager->RequestCommandAllocatorLists(m_pCommandAllocators, m_pCommandLists, type, numCommandALs);

			for (size_t i = CurrentAllocatorSize; i < numCommandALs; ++i)
			{
				ASSERT_HR(m_pCommandLists[i]->Close());
			}
		}
		else if (numCommandALs < CurrentAllocatorSize) // 1 < 4 /// 3 < 6 // 7 < 15
		{
			size_t NumDiscards = CurrentAllocatorSize - numCommandALs;

			Queue.discardAllocators(m_pCommandAllocators, m_LastExecuteFenceValue, NumDiscards);

			for (size_t i = numCommandALs; i < CurrentAllocatorSize; ++i) // for (size_t i = NumDiscards; i < CurrentAllocatorSize; ++i)
			{
				CHECK_IUNKOWN_REF_COUNT_DEBUG(SafeRelease(m_pCommandLists[i]));
			}

			m_pCommandLists.erase(m_pCommandLists.begin() + numCommandALs, m_pCommandLists.end()); // m_pCommandLists.erase(m_pCommandLists.end() - NumDiscards, m_pCommandLists.end());
			m_pCommandLists.shrink_to_fit();
		}
		
		for (size_t i = 0ul; i < numCommandALs; ++i)
		{
			ASSERT_HR(m_pCommandLists[i]->Reset(m_pCommandAllocators[i], nullptr));
		}

		m_bRecordingFlag = (1ull << 63) + ((1ull << numCommandALs) - 1);

		m_type               = type;
		m_NumCommandPair     = numCommandALs;
		m_numStandByBarriers = 0;
		m_LastExecuteFenceValue = Queue.GetLastCompletedFenceValue();

		// GPUTaskFiber can be recycled?
		m_GPUTaskFiberContexts.clear();
		m_GPUTaskFiberContexts.shrink_to_fit();
		m_GPUTaskFiberContexts.reserve(m_NumCommandPair);
		m_GPUTaskFiberContexts.resize(m_NumCommandPair, *this);

		for (uint8_t i = 0; i < m_NumCommandPair; ++i)
		{
			setCurrentDescriptorHeaps(i);
		}
	}

	void CommandContext::SubmitExternalResourceBarriers(const D3D12_RESOURCE_BARRIER pBarriers[], const UINT numBarriers, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX && (numBarriers < NUM_LIMIT_RESOURCE_BARRIER_16));
		if (0 < numBarriers)
		{
			m_pCommandLists[commandIndex]->ResourceBarrier(numBarriers, pBarriers);
			INCRE_DEBUG_SET_RESOURCE_BARRIER_COUNT;
		}
	}

	void CommandContext::SubmitResourceBarriers(const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		if (0 < m_numStandByBarriers)
		{
			m_pCommandLists[commandIndex]->ResourceBarrier(m_numStandByBarriers, m_resourceBarriers);
			m_numStandByBarriers = 0U;
			INCRE_DEBUG_SET_RESOURCE_BARRIER_COUNT;
		}
	}

	void CommandContext::SplitResourceTransition(GPUResource& targetResource, D3D12_RESOURCE_STATES statePending)
	{
		ASSERT(targetResource.GetResource() && (m_numStandByBarriers < NUM_LIMIT_RESOURCE_BARRIER_16));

		// If it's already transitioning, finish that transition
		if (targetResource.m_pendingState != (D3D12_RESOURCE_STATES)-1)
		{
			TransitionResource(targetResource, targetResource.m_pendingState);
		}

		D3D12_RESOURCE_STATES OldState = targetResource.m_currentState;

		if (OldState != statePending)
		{
			D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarriers[m_numStandByBarriers++];

			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			BarrierDesc.Transition.pResource   = targetResource.GetResource();
			BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			BarrierDesc.Transition.StateBefore = OldState;
			BarrierDesc.Transition.StateAfter  = statePending;

			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

			targetResource.m_pendingState = statePending;
		}
	}

	// TODO : To change to find unused ResourceBarrier, then changed it. 
	void CommandContext::TransitionResource(GPUResource& targetResource, D3D12_RESOURCE_STATES stateAfter, D3D12_RESOURCE_STATES forcedOldState)
	{
		ASSERT(targetResource.GetResource() && (m_numStandByBarriers < NUM_LIMIT_RESOURCE_BARRIER_16));

		D3D12_RESOURCE_STATES OldState = (forcedOldState == (D3D12_RESOURCE_STATES)-1) ? targetResource.m_currentState : forcedOldState;

		if (OldState != stateAfter)
		{
			D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarriers[m_numStandByBarriers++];

			BarrierDesc.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			BarrierDesc.Transition.pResource   = targetResource.GetResource();
			BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			BarrierDesc.Transition.StateBefore = OldState;
			BarrierDesc.Transition.StateAfter  = stateAfter;

			// Check to see if we already started the transition
			if (targetResource.m_pendingState != D3D12_RESOURCE_STATES(-1)) // <- Before : if(NewState == Resource.m_pendingState)
			{
				ASSERT(stateAfter == targetResource.m_pendingState);
				BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
				targetResource.m_pendingState = (D3D12_RESOURCE_STATES)-1;
			}
			else
			{
				BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			}

			targetResource.m_currentState = stateAfter;
		}
		else if (stateAfter == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			InsertUAVBarrier(targetResource);
		}
	}
	void CommandContext::InsertUAVBarrier(GPUResource& targetResource)
	{
		ASSERT(m_numStandByBarriers < NUM_LIMIT_RESOURCE_BARRIER_16, "Exceeded arbitrary limit on buffered barriers");
		D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarriers[m_numStandByBarriers++];

		BarrierDesc.Type  = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.UAV.pResource = targetResource.GetResource();
	}
	void CommandContext::InsertAliasBarrier(GPUResource& beforeResource, GPUResource& afterResource)
	{
		ASSERT(m_numStandByBarriers < NUM_LIMIT_RESOURCE_BARRIER_16, "Exceeded arbitrary limit on buffered barriers");
		D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarriers[m_numStandByBarriers++];

		BarrierDesc.Type  = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Aliasing.pResourceBefore = beforeResource.GetResource();
		BarrierDesc.Aliasing.pResourceAfter  = afterResource.GetResource();
	}

	void CommandContext::SetPipelineState(const PSO& customPSO, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		ID3D12PipelineState* PipelineState = customPSO.GetPipelineStateObject();
		if (PipelineState == m_GPUTaskFiberContexts[commandIndex].pCurrentPSO)
		{
			return;
		}

		m_pCommandLists[commandIndex]->SetPipelineState(PipelineState);
		m_GPUTaskFiberContexts[commandIndex].pCurrentPSO = PipelineState;
		INCRE_DEBUG_SET_PSO_COUNT;
	}
	void CommandContext::SetPipelineStateRange(const PSO& customPSO, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(CHECK_VALID_RANGE);

		for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
		{
			ID3D12PipelineState* PipelineState = customPSO.GetPipelineStateObject();
			if (PipelineState == m_GPUTaskFiberContexts[i].pCurrentPSO)
			{
				continue;
			}

			m_pCommandLists[i]->SetPipelineState(PipelineState);
			m_GPUTaskFiberContexts[i].pCurrentPSO = PipelineState;
			INCRE_DEBUG_SET_PSO_COUNT_I;
		}
	}

	void CommandContext::SetPipelineStateByPtr(ID3D12PipelineState* const pPSO, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		if (pPSO == m_GPUTaskFiberContexts[commandIndex].pCurrentPSO)
		{
			return;
		}

		m_pCommandLists[commandIndex]->SetPipelineState(pPSO);
		m_GPUTaskFiberContexts[commandIndex].pCurrentPSO = pPSO;
		INCRE_DEBUG_SET_PSO_COUNT;
	}
	void CommandContext::SetPipelineStateByPtrRange(ID3D12PipelineState* const pPSO, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(CHECK_VALID_RANGE);
		for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
		{
			if (pPSO == m_GPUTaskFiberContexts[i].pCurrentPSO)
			{
				continue;
			}

			m_pCommandLists[i]->SetPipelineState(pPSO);
			m_GPUTaskFiberContexts[i].pCurrentPSO = pPSO;
			INCRE_DEBUG_SET_PSO_COUNT_I;
		}
	}

	void CommandContext::setDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* const pHeap, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		if (m_GPUTaskFiberContexts[commandIndex].pCurrentDescriptorHeaps[heapType] != pHeap)
		{
			m_GPUTaskFiberContexts[commandIndex].pCurrentDescriptorHeaps[heapType] = pHeap;
			setCurrentDescriptorHeaps(commandIndex);
		}
	}
	void CommandContext::setDescriptorHeaps(const UINT numHeap, const D3D12_DESCRIPTOR_HEAP_TYPE heapTypes[], ID3D12DescriptorHeap* const pHeaps[], const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		bool bAnyChanged = false;

		for (size_t i = 0; i < numHeap; ++i)
		{
			if (m_GPUTaskFiberContexts[commandIndex].pCurrentDescriptorHeaps[heapTypes[i]] != pHeaps[i])
			{
				m_GPUTaskFiberContexts[commandIndex].pCurrentDescriptorHeaps[heapTypes[i]] = pHeaps[i];
				bAnyChanged = true;
			}
		}

		if (bAnyChanged)
		{
			setCurrentDescriptorHeaps(commandIndex);
		}
	}

	void CommandContext::InsertTimeStamp(ID3D12QueryHeap* const pQueryHeap, const UINT QueryIdx, const uint8_t commandIndex) const
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		// For Gpu Profiling.
		m_pCommandLists[commandIndex]->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
	}

	void CommandContext::ResolveTimeStamps(ID3D12Resource* const pReadbackHeap, ID3D12QueryHeap* const pQueryHeap, const UINT NumQueries, const uint8_t commandIndex) const
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		// For Gpu Profiling.
		m_pCommandLists[commandIndex]->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
	}

	void CommandContext::PIXBeginEvent(const wchar_t* const label, const uint8_t commandIndex) const
	{
#if defined(_DEBUG) | !defined(RELEASE)
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		::PIXBeginEvent(m_pCommandLists[commandIndex], 0, label);
#endif
	}
	void CommandContext::PIXEndEvent(const uint8_t commandIndex) const
	{
#if defined(_DEBUG) | !defined(RELEASE)
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		::PIXEndEvent(m_pCommandLists[commandIndex]);
#endif
	}

	void CommandContext::PIXBeginEventAll(const wchar_t* const label) const
	{
#if defined(_DEBUG) | !defined(RELEASE)
		for (auto& e : m_pCommandLists)
		{
			::PIXBeginEvent(e, 0, label);
		}
#endif
	}
	void CommandContext::PIXEndEventAll() const
	{
#if defined(_DEBUG) | !defined(RELEASE)
		for (auto& e : m_pCommandLists)
		{
			::PIXEndEvent(e);
		}
#endif
	}

	void CommandContext::PIXSetMarker(const wchar_t* const label, const uint8_t commandIndex) const
	{
#if defined(_DEBUG) | !defined(RELEASE)
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		::PIXSetMarker(m_pCommandLists[commandIndex], 0, label);
#endif
	}
	void CommandContext::PIXSetMarkerAll(const wchar_t* const label) const
	{
#if defined(_DEBUG) | !defined(RELEASE)
		for (auto& e : m_pCommandLists)
		{
			::PIXSetMarker(e, 0, label);
		}
#endif
	}

	void CommandContext::setCurrentDescriptorHeaps(const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		UINT NonNullHeaps = 0;
		ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		for (size_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			ID3D12DescriptorHeap* HeapIter = m_GPUTaskFiberContexts[commandIndex].pCurrentDescriptorHeaps[i];
			if (HeapIter != nullptr)
			{
				HeapsToBind[NonNullHeaps++] = HeapIter;
			}
		}

		if (0 < NonNullHeaps)
		{
			m_pCommandLists[commandIndex]->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
			INCRE_DEBUG_SET_DESCRIPTOR_HEAPS_COUNT;
		}
	}

	void CommandContext::SetPredication(ID3D12Resource* const Buffer, const UINT64 BufferOffset, const D3D12_PREDICATION_OP Op, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		m_pCommandLists[commandIndex]->SetPredication(Buffer, BufferOffset, Op);
	}

#pragma region CAMERA_LIGHTS
	void CommandContext::SetModelToProjection(const Math::Matrix4& _ViewProjMatrix)
	{
		m_VSConstants.modelToProjection = _ViewProjMatrix;
	}
	
	void CommandContext::SetModelToProjectionByCamera(const IBaseCamera& _IBaseCamera)
	{
		m_VSConstants.modelToProjection = _IBaseCamera.GetViewProjMatrix();
		m_VSConstants.ViewMatrix        = _IBaseCamera.GetViewMatrix();
 		DirectX::XMStoreFloat3(&m_VSConstants.viewerPos, _IBaseCamera.GetPosition());
	}

	void CommandContext::SetModelToShadow(const Math::Matrix4& _ShadowMatrix)
	{
		m_VSConstants.modelToShadow = _ShadowMatrix;
	}

	void CommandContext::SetModelToShadowByShadowCamera(ShadowCamera& _ShadowCamera)
	{
		m_pMainLightShadowCamera = &_ShadowCamera;
		SetModelToShadow(m_pMainLightShadowCamera->GetShadowMatrix());
	}

	void CommandContext::SetMainCamera(Camera& _Camera)
	{
		m_pMainCamera = &_Camera;
		SetModelToProjectionByCamera(_Camera);
	}

	Camera* CommandContext::GetpMainCamera() const
	{
		ASSERT(m_pMainCamera != nullptr);
		return m_pMainCamera;
	}

	VSConstants CommandContext::GetVSConstants() const
	{
		return m_VSConstants;
	}
	PSConstants CommandContext::GetPSConstants() const
	{
		return m_PSConstants;
	}
	
	void CommandContext::SetMainLightDirection(const Math::Vector3& MainLightDirection)
	{
		m_PSConstants.sunDirection = MainLightDirection;
	}

	void CommandContext::SetMainLightColor(const Math::Vector3& Color, const float Intensity)
	{
		m_PSConstants.sunLightColor = Color * Intensity;
	}

	void CommandContext::SetAmbientLightColor(const Math::Vector3& Color, const float Intensity)
	{
		m_PSConstants.ambientColor = Color * Intensity;
	}

	void CommandContext::SetShadowTexelSize(const float TexelSize)
	{
		ASSERT(0.0f < TexelSize && TexelSize < 1.0f);
		m_PSConstants.ShadowTexelSize[0] = TexelSize;
		m_PSConstants.ShadowTexelSize[1] = TexelSize;
	}

	void CommandContext::SetTileDimension(const uint32_t MainColorBufferWidth, const uint32_t MainColorBufferHeight, const uint32_t LightWorkGroupSize)
	{
		ASSERT(1u < LightWorkGroupSize);

		m_PSConstants.InvTileDim[0] = 1.0f / LightWorkGroupSize;
		m_PSConstants.InvTileDim[1] = m_PSConstants.InvTileDim[0];

		m_PSConstants.TileCount[0] = HashInternal::DivideByMultiple(MainColorBufferWidth, LightWorkGroupSize);
		m_PSConstants.TileCount[0] = HashInternal::DivideByMultiple(MainColorBufferHeight, LightWorkGroupSize);
	}

	void CommandContext::SetSpecificLightIndex(const uint32_t FirstConeLightIndex, const uint32_t FirstConeShadowedLightIndex)
	{
		m_PSConstants.FirstLightIndex[0] = FirstConeLightIndex;
		m_PSConstants.FirstLightIndex[1] = FirstConeShadowedLightIndex;
	}

	void CommandContext::SetPSConstants(MainLight& _MainLight)
	{
		SetMainLightDirection(_MainLight.GetMainLightDirection());
		SetMainLightColor(_MainLight.m_LightColor, _MainLight.m_LightIntensity);
		SetAmbientLightColor(_MainLight.m_AmbientColor, _MainLight.m_AmbientIntensity);
	}

	void CommandContext::SetSync()
	{
		device::g_commandContextManager.m_VSConstants       = m_VSConstants;
		device::g_commandContextManager.m_PSConstants       = m_PSConstants;
		device::g_commandContextManager.m_pMainCamera       = m_pMainCamera;
		device::g_commandContextManager.m_pMainShadowCamera = m_pMainLightShadowCamera;
	}

#pragma endregion CAMERA_LIGHTS

#pragma endregion COMMAND_CONTEXT

#pragma region GRAPHICS_CONTEXT

	STATIC GraphicsContext& GraphicsContext::Begin(const uint8_t numCommandALs)
	{
		ASSERT((numCommandALs < COMMAND_LIMIT));

		GraphicsContext* NewContext = reinterpret_cast<GraphicsContext*>(device::g_commandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT, numCommandALs));
		return *NewContext;
	}

	STATIC GraphicsContext& GraphicsContext::Resume(const uint64_t contextID)
	{
		custom::CommandContext* ret = nullptr;
		ret = device::g_commandContextManager.GetRecordingContext(D3D12_COMMAND_LIST_TYPE_DIRECT, contextID);
		ASSERT(ret && ret->GetType() == D3D12_COMMAND_LIST_TYPE_DIRECT);
		return *reinterpret_cast<custom::GraphicsContext*>(ret);
	}

	STATIC GraphicsContext& GraphicsContext::GetRecentContext()
	{
		custom::CommandContext* ret = nullptr;
		ret = device::g_commandContextManager.GetRecentContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
		ASSERT(ret && ret->GetType() == D3D12_COMMAND_LIST_TYPE_DIRECT);
		return *reinterpret_cast<custom::GraphicsContext*>(ret);
	}

	// TODO : Eliminate Overloading.
	void GraphicsContext::SetRootSignature(const RootSignature& customRS, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		if (customRS.GetSignature() == m_GPUTaskFiberContexts[commandIndex].pCurrentGraphicsRS)
		{
			return;
		}

		m_pCommandLists[commandIndex]->SetGraphicsRootSignature(m_GPUTaskFiberContexts[commandIndex].pCurrentGraphicsRS = customRS.GetSignature());
		INCRE_DEBUG_SET_RS_COUNT;

		m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.ParseGraphicsRootSignature(customRS);
		m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.ParseGraphicsRootSignature(customRS);
	}

	void GraphicsContext::SetRootSignatureRange(const RootSignature& customRS, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(CHECK_VALID_RANGE);

		for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
		{
			if (customRS.GetSignature() == m_GPUTaskFiberContexts[i].pCurrentGraphicsRS)
			{
				continue;
			}

			m_pCommandLists[i]->SetGraphicsRootSignature(m_GPUTaskFiberContexts[i].pCurrentGraphicsRS = customRS.GetSignature());
			INCRE_DEBUG_SET_RS_COUNT_I;

			m_GPUTaskFiberContexts[i].DynamicViewDescriptorHeaps.ParseGraphicsRootSignature(customRS);
			m_GPUTaskFiberContexts[i].DynamicSamplerDescriptorHeaps.ParseGraphicsRootSignature(customRS);
		}
	}

	void GraphicsContext::SetRenderTargets(const UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV);
	}

	void GraphicsContext::SetRenderTargetsRange(const UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], const D3D12_CPU_DESCRIPTOR_HANDLE DSV, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(CHECK_VALID_RANGE);

		for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
		{
			m_pCommandLists[i]->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV);
		}
	}

	void GraphicsContext::SetRenderTargets(const UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
	}

	void GraphicsContext::BeginQuery(ID3D12QueryHeap* const QueryHeap, const D3D12_QUERY_TYPE Type, UINT HeapIndex, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->BeginQuery(QueryHeap, Type, HeapIndex);
	}

	void GraphicsContext::EndQuery(ID3D12QueryHeap* const QueryHeap, const D3D12_QUERY_TYPE Type, const UINT HeapIndex, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->EndQuery(QueryHeap, Type, HeapIndex);
	}

	void GraphicsContext::ResolveQueryData
	(
		ID3D12QueryHeap* const QueryHeap, 
		const D3D12_QUERY_TYPE Type, 
		const UINT StartIndex, 
		const UINT NumQueries, 
		ID3D12Resource* const DestinationBuffer, 
		const UINT64 DestinationBufferOffset, 
		const uint8_t commandIndex
	)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);
	}

	void GraphicsContext::ClearUAV(const UAVBuffer& Target, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV 
		// (because it essentially runs a shader to set all of the values).
		D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.UploadDirect(Target.GetUAV(), commandIndex);
		const UINT ClearColor[4] = {};
		m_pCommandLists[commandIndex]->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void GraphicsContext::ClearUAV(const ColorBuffer& Target, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV 
		// (because it essentially runs a shader to set all of the values).
		D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.UploadDirect(Target.GetUAV(), commandIndex);
		CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

		const float* ClearColor = Target.GetClearColor().GetPtr();
		m_pCommandLists[commandIndex]->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void GraphicsContext::ClearColor(const ColorBuffer& Target, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		m_pCommandLists[commandIndex]->ClearRenderTargetView(Target.GetRTV(), Target.GetClearColor().GetPtr(), 0, nullptr);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void GraphicsContext::ClearDepth(const DepthBuffer& Target, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		m_pCommandLists[commandIndex]->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void GraphicsContext::ClearStencil(const DepthBuffer& Target, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		m_pCommandLists[commandIndex]->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void GraphicsContext::ClearDepthAndStencil(const DepthBuffer& Target, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		m_pCommandLists[commandIndex]->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void GraphicsContext::SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, const uint8_t commandIndex)
	{
		ASSERT(rect.left <= rect.right && rect.top <= rect.bottom && CHECK_VALID_COMMAND_INDEX);

		m_pCommandLists[commandIndex]->RSSetViewports(1, &vp);
		m_pCommandLists[commandIndex]->RSSetScissorRects(1, &rect);
	}
	void GraphicsContext::SetViewportAndScissorRange(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(rect.left <= rect.right && rect.top <= rect.bottom && CHECK_VALID_RANGE);

		for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
		{
			m_pCommandLists[i]->RSSetViewports(1, &vp);
			m_pCommandLists[i]->RSSetScissorRects(1, &rect);
		}
	}
	void GraphicsContext::SetViewport(const D3D12_VIEWPORT& vp, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->RSSetViewports(1, &vp);
	}
	void GraphicsContext::SetViewport(const FLOAT x, const FLOAT y, const FLOAT w, const FLOAT h, const FLOAT minDepth, const FLOAT maxDepth, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		D3D12_VIEWPORT vp;
		vp.Width    = w;
		vp.Height   = h;
		vp.MinDepth = minDepth;
		vp.MaxDepth = maxDepth;
		vp.TopLeftX = x;
		vp.TopLeftY = y;

		m_pCommandLists[commandIndex]->RSSetViewports(1, &vp);
	}

	void GraphicsContext::SetScissor(const D3D12_RECT& rect, const uint8_t commandIndex)
	{
		ASSERT(rect.left < rect.right && rect.top < rect.bottom && CHECK_VALID_COMMAND_INDEX);
		device::g_commandContextManager.m_Scissor = rect;
		m_pCommandLists[commandIndex]->RSSetScissorRects(1, &rect);
	}
	void GraphicsContext::SetScissor(const LONG left, const LONG top, const LONG right, const LONG bottom, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		SetScissor(CD3DX12_RECT(left, top, right, bottom), commandIndex);
	}

	void GraphicsContext::SetViewportAndScissor(const LONG x, const LONG y, const LONG w, const LONG h, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		SetViewport((float)x, (float)y, (float)w, (float)h, 0.0f, 1.0f, commandIndex);
		SetScissor(x, y, x + w, y + h, commandIndex);
	}
	void GraphicsContext::SetViewportAndScissor(const PixelBuffer& TargetBuffer, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);

		D3D12_VIEWPORT vp;
		D3D12_RECT RECT;
		vp.Width    = (float)TargetBuffer.GetWidth();
		vp.Height   = (float)TargetBuffer.GetHeight();
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		RECT.left   = 0L;
		RECT.top    = 0L;
		RECT.right  = (LONG)TargetBuffer.GetWidth();
		RECT.bottom = (LONG)TargetBuffer.GetHeight();

		SetViewportAndScissor(vp, RECT, commandIndex);
	}
	void GraphicsContext::SetViewportAndScissorRange(const PixelBuffer& TargetBuffer, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(CHECK_VALID_RANGE);

		D3D12_VIEWPORT vp;
		D3D12_RECT RECT;
		vp.Width = (float)TargetBuffer.GetWidth();
		vp.Height = (float)TargetBuffer.GetHeight();
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		RECT.left   = 0L;
		RECT.top    = 0L;
		RECT.right  = (LONG)TargetBuffer.GetWidth();
		RECT.bottom = (LONG)TargetBuffer.GetHeight();

		SetViewportAndScissorRange(vp, RECT, startCommandIndex, endCommandIndex);
	}

	void GraphicsContext::SetPrimitiveTopology(const D3D12_PRIMITIVE_TOPOLOGY Topology, bool bForced)
	{
		if ((m_PrimitiveTopology != Topology) || bForced)
		{
			for (auto& e : m_pCommandLists)
			{
				e->IASetPrimitiveTopology(Topology);
			}

			m_PrimitiveTopology = Topology;
		}
	}

	void GraphicsContext::SetConstants(const UINT RootIndex, float _32BitParameter0, float _32BitParameter1, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->SetGraphicsRoot32BitConstant(RootIndex, *reinterpret_cast<UINT*>(&_32BitParameter0), 0);
		m_pCommandLists[commandIndex]->SetGraphicsRoot32BitConstant(RootIndex, *reinterpret_cast<UINT*>(&_32BitParameter1), 1);
		INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
	}

	void GraphicsContext::SetConstants(const UINT RootIndex, float _32BitParameter0, float _32BitParameter1, float _32BitParameter2, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->SetGraphicsRoot32BitConstant(RootIndex, *reinterpret_cast<UINT*>(&_32BitParameter0), 0);
		m_pCommandLists[commandIndex]->SetGraphicsRoot32BitConstant(RootIndex, *reinterpret_cast<UINT*>(&_32BitParameter1), 1);
		m_pCommandLists[commandIndex]->SetGraphicsRoot32BitConstant(RootIndex, *reinterpret_cast<UINT*>(&_32BitParameter2), 2);
		INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
	}

	void GraphicsContext::SetDynamicConstantBufferView(const UINT RootIndex, const size_t BufferSize, const void* const BufferData, const uint8_t commandIndex)
	{
		ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16) && CHECK_VALID_COMMAND_INDEX);
		LinearBuffer ConstantBuffer = m_CPULinearAllocator.Allocate(BufferSize);
		//SIMDMemCopy(ConstantBuffer.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
		memcpy(ConstantBuffer.pData, BufferData, BufferSize);
		m_pCommandLists[commandIndex]->SetGraphicsRootConstantBufferView(RootIndex, ConstantBuffer.GPUAddress);
		INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
	}
	void GraphicsContext::SetDynamicConstantBufferViewRange(const UINT RootIndex, const size_t BufferSize, const void* const BufferData, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16) && CHECK_VALID_RANGE);
		LinearBuffer ConstantBuffer = m_CPULinearAllocator.Allocate(BufferSize);
		//SIMDMemCopy(ConstantBuffer.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
		memcpy(ConstantBuffer.pData, BufferData, BufferSize);

		for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
		{
			m_pCommandLists[i]->SetGraphicsRootConstantBufferView(RootIndex, ConstantBuffer.GPUAddress);
			INCRE_DEBUG_SET_ROOT_PARAM_COUNT_I;
		}
	}

	void GraphicsContext::SetDynamicVB(const UINT Slot, const size_t NumVertices, const size_t VertexStride, const void* VertexData, const uint8_t commandIndex)
	{
		ASSERT(VertexData != nullptr && HashInternal::IsAligned(VertexData, 16) && CHECK_VALID_COMMAND_INDEX);

		size_t BufferSize = HashInternal::AlignUp(NumVertices * VertexStride, 16);
		LinearBuffer VertexBuffer = m_CPULinearAllocator.Allocate(BufferSize);

		SIMDMemCopy(VertexBuffer.pData, VertexData, BufferSize >> 4);

		D3D12_VERTEX_BUFFER_VIEW VBView;
		VBView.BufferLocation = VertexBuffer.GPUAddress;
		VBView.SizeInBytes = (UINT)BufferSize;
		VBView.StrideInBytes = (UINT)VertexStride;

		m_pCommandLists[commandIndex]->IASetVertexBuffers(Slot, 1, &VBView);
	}

	void GraphicsContext::SetDynamicIB(const size_t IndexCount, const uint16_t* const IndexData, const uint8_t commandIndex)
	{
		ASSERT(IndexData != nullptr && HashInternal::IsAligned(IndexData, 16) && CHECK_VALID_COMMAND_INDEX);

		size_t BufferSize = HashInternal::AlignUp(IndexCount * sizeof(uint16_t), 16);
		LinearBuffer IndexBuffer = m_CPULinearAllocator.Allocate(BufferSize);

		SIMDMemCopy(IndexBuffer.pData, IndexData, BufferSize >> 4);

		D3D12_INDEX_BUFFER_VIEW IBView;
		IBView.BufferLocation = IndexBuffer.GPUAddress;
		IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
		IBView.Format = DXGI_FORMAT_R16_UINT;

		m_pCommandLists[commandIndex]->IASetIndexBuffer(&IBView);
	}

	void GraphicsContext::SetDynamicSRV(const UINT RootIndex, const size_t BufferSize, const void* const BufferData, const uint8_t commandIndex)
	{
		ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16) && CHECK_VALID_COMMAND_INDEX);
		LinearBuffer temp = m_CPULinearAllocator.Allocate(BufferSize);
		SIMDMemCopy(temp.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);
		m_pCommandLists[commandIndex]->SetGraphicsRootShaderResourceView(RootIndex, temp.GPUAddress);
		INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
	}
	void GraphicsContext::SetDynamicSRVRange(const UINT RootIndex, const size_t BufferSize, const void* const BufferData, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(BufferData != nullptr && HashInternal::IsAligned(BufferData, 16) && CHECK_VALID_RANGE);
		LinearBuffer temp = m_CPULinearAllocator.Allocate(BufferSize);
		SIMDMemCopy(temp.pData, BufferData, HashInternal::AlignUp(BufferSize, 16) >> 4);

		for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
		{
			m_pCommandLists[i]->SetGraphicsRootShaderResourceView(RootIndex, temp.GPUAddress);
			INCRE_DEBUG_SET_ROOT_PARAM_COUNT_I;
		}
	}

	void GraphicsContext::SetBufferSRV(const UINT RootIndex, const UAVBuffer& SRV, const UINT64 Offset, const uint8_t commandIndex)
	{
		ASSERT(((SRV.m_currentState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0) && CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
		INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
	}

	void GraphicsContext::SetBufferUAV(const UINT RootIndex, const UAVBuffer& UAV, const UINT64 Offset, const uint8_t commandIndex)
	{
		ASSERT(((UAV.m_currentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0) && CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
		INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
	}

	// Texture -> FundamentalTexture 
	void GraphicsContext::SetDynamicDescriptor(const UINT RootIndex, const UINT Offset, const D3D12_CPU_DESCRIPTOR_HANDLE Handle, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		SetDynamicDescriptors(RootIndex, Offset, 1, &Handle, commandIndex);
	}

	void GraphicsContext::SetDynamicDescriptors(const UINT RootIndex, const UINT Offset, const UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[], const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
	}


	void GraphicsContext::SetDynamicDescriptorsRange(const UINT RootIndex, const UINT Offset, const UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[], const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(CHECK_VALID_RANGE);
		for (size_t i = startCommandIndex; i <= endCommandIndex; ++i)
		{
			m_GPUTaskFiberContexts[i].DynamicViewDescriptorHeaps.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
		}
	}

	void GraphicsContext::SetDynamicSampler(const UINT RootIndex, const UINT Offset, const D3D12_CPU_DESCRIPTOR_HANDLE Handle, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		SetDynamicSamplers(RootIndex, Offset, 1, &Handle, commandIndex);
	}

	void GraphicsContext::SetDynamicSamplers(const UINT RootIndex, const UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[], const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
	}

	void GraphicsContext::SetDescriptorTable(const UINT RootIndex, const D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
		INCRE_DEBUG_SET_ROOT_PARAM_COUNT;
	}

	void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->IASetIndexBuffer(&IBView);
	}

	void GraphicsContext::SetVertexBuffer(const UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		SetVertexBuffers(Slot, 1, &VBView, commandIndex);
	}

	void GraphicsContext::SetVertexBuffers(const UINT StartSlot, const UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[], const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		m_pCommandLists[commandIndex]->IASetVertexBuffers(StartSlot, Count, VBViews);
	}

	void GraphicsContext::SetVSConstantsBuffer(const UINT RootIndex, const uint8_t commandIndex)
	{
		SetDynamicConstantBufferView(RootIndex, sizeof(m_VSConstants), &m_VSConstants, commandIndex);
	}

	void GraphicsContext::SetVSConstantsBufferRange(const UINT RootIndex, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(CHECK_VALID_RANGE);
		SetDynamicConstantBufferViewRange(RootIndex, sizeof(m_VSConstants), &m_VSConstants, startCommandIndex, endCommandIndex);
	}

	void GraphicsContext::SetPSConstantsBuffer(const UINT RootIndex, const uint8_t commandIndex)
	{
		SetDynamicConstantBufferView(RootIndex, sizeof(m_PSConstants), &m_PSConstants, commandIndex);
	}

	void GraphicsContext::SetPSConstantsBufferRange(const UINT RootIndex, const uint8_t startCommandIndex, const uint8_t endCommandIndex)
	{
		ASSERT(CHECK_VALID_RANGE);
		SetDynamicConstantBufferViewRange(RootIndex, sizeof(m_PSConstants), &m_PSConstants, startCommandIndex, endCommandIndex);
	}

	void GraphicsContext::Draw(const UINT VertexCount, const UINT VertexStartOffset, const uint8_t commandIndex)
	{
		DrawInstanced(VertexCount, 1U, VertexStartOffset, 0U, commandIndex);
	}

	void GraphicsContext::DrawIndexed(const UINT IndexCount, const UINT StartIndexLocation/*= 0*/, const INT BaseVertexLocation/*= 0*/, const uint8_t commandIndex)
	{
		DrawIndexedInstanced(IndexCount, 1U, StartIndexLocation, BaseVertexLocation, 0U, commandIndex);
	}

	void GraphicsContext::DrawInstanced
	(
		const UINT VertexCountPerInstance, const UINT InstanceCount,
		const UINT StartVertexLocation/*= 0*/, const UINT StartInstanceLocation,/*= 0*/
		const uint8_t commandIndex
	)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		// SubmitResourceBarriers(commandIndex);
		m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.SetGraphicsRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex); // -> ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
		m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.SetGraphicsRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
		m_pCommandLists[commandIndex]->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
		INCRE_DEBUG_DRAW_JOB_COUNT;
	}
	
	void GraphicsContext::DrawIndexedInstanced(const UINT IndexCountPerInstance, const UINT InstanceCount, const UINT StartIndexLocation,
		const INT BaseVertexLocation, const UINT StartInstanceLocation, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		// SubmitResourceBarriers(commandIndex);
		m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.SetGraphicsRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
		m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.SetGraphicsRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
		m_pCommandLists[commandIndex]->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
		INCRE_DEBUG_DRAW_JOB_COUNT;
	}

	void GraphicsContext::ExecuteIndirect
	(
		const CommandSignature& CommandSig,
		const UAVBuffer& ArgumentBuffer, const uint64_t ArgumentStartOffset,
		const uint32_t MaxCommands, const UAVBuffer* const CommandCounterBuffer, const uint64_t CounterOffset,
		const uint8_t commandIndex
	)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		// SubmitResourceBarriers(commandIndex);
		m_GPUTaskFiberContexts[commandIndex].DynamicViewDescriptorHeaps.SetGraphicsRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
		m_GPUTaskFiberContexts[commandIndex].DynamicSamplerDescriptorHeaps.SetGraphicsRootDescriptorTables(m_pCommandLists[commandIndex], commandIndex);
		m_pCommandLists[commandIndex]->ExecuteIndirect
		(
			CommandSig.GetSignature(), MaxCommands,
			ArgumentBuffer.GetResource(), ArgumentStartOffset,
			CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset
		);
	}

	void GraphicsContext::DrawIndirect(const UAVBuffer& ArgumentBuffer, const uint64_t ArgumentBufferOffset, const uint8_t commandIndex)
	{
		ExecuteIndirect(graphics::g_DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset, commandIndex);
	}

	void GraphicsContext::ExecuteBundle(ID3D12GraphicsCommandList* const pBundle, const uint8_t commandIndex)
	{
		ASSERT(pBundle->GetType() == D3D12_COMMAND_LIST_TYPE_BUNDLE &&
			CHECK_VALID_COMMAND_INDEX);
		// Bundle도 Indirect처럼 Capuslation해야되나...
		m_pCommandLists[commandIndex]->ExecuteBundle(pBundle);
	}
#pragma endregion GRAPHICS_CONTEXT
}
