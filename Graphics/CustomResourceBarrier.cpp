#include "stdafx.h"
#include "CustomResourceBarrier.h"
#include "CommandContext.h"
#include "GPUResource.h"

namespace custom
{
	constexpr uint8_t NUM_LIMIT_RESOURCE_BARRIER_16 = 16u;

	ResourceBarrier::ResourceBarrier()
		:
		m_NumBarrier(0u)
	{
		ZeroMemory(m_ResourceBarrier, _countof(m_ResourceBarrier) * sizeof(D3D12_RESOURCE_BARRIER));
	}

	void ResourceBarrier::Bind(custom::CommandContext& baseContext, uint8_t commandIndex) DEBUG_EXCEPT
	{
		baseContext.SubmitExternalResourceBarriers(m_ResourceBarrier, m_NumBarrier, commandIndex);
	}

	void ResourceBarrier::InsertResourceUAVBarrier(GPUResource& resource, uint8_t barrierIndex)
	{
		D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrier[barrierIndex];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.UAV.pResource = resource.GetResource();
	}
	void ResourceBarrier::InsertResourceAliasingBarrier(GPUResource& resource, GPUResource& after, uint8_t barrierIndex)
	{
		ASSERT(false); // 지금 안되는 이유 : Insert하는 시점과, Bind되는 시점의 Resource_STATE가 달라서 위험함.
		               // 어떻게 고쳐야되냐면 Insert할 때는 리소스 TransitionBarrier순서만 적어놓고 바인드 될때, 
		               // 그 때 GPUResource의 상태를 가져와서 D3D12_RESOURCE_BARRIER작성하고 commandList에 기록.
		D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrier[barrierIndex];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Aliasing.pResourceBefore = resource.GetResource();
		BarrierDesc.Aliasing.pResourceAfter = after.GetResource();
	}
	void ResourceBarrier::InsertResourceTransitionBarrier
	(
		GPUResource& resource, const D3D12_RESOURCE_STATES newState, 
		const uint8_t barrierIndex, UINT subResources /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/
	)
	{
		ASSERT(false);
		ASSERT(resource.GetResource() && (resource.m_currentState != newState));
		
		D3D12_RESOURCE_STATES OldState = resource.m_currentState;

		if (OldState != newState)
		{
			D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrier[barrierIndex];

			BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			BarrierDesc.Transition.pResource = resource.GetResource();
			BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			BarrierDesc.Transition.StateBefore = resource.m_currentState;
			BarrierDesc.Transition.StateAfter = newState;

			if (newState == resource.m_pendingState)
			{
				BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
				resource.m_pendingState = (D3D12_RESOURCE_STATES)-1;
			}
			else
			{
				BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			}

			resource.m_currentState = newState;
		}
		else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			InsertResourceUAVBarrier(resource, barrierIndex);
		}
	}
}