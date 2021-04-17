#include "stdafx.h"
#include "ManagedD3D12Heap.h"
#include "d3dx12Residency.h"
#include "Device.h"

namespace custom
{
	ManagedD3D12Heap::~ManagedD3D12Heap()
	{
		Destroy();
	}

	void ManagedD3D12Heap::Destroy()
	{
		// TODO: Check ref counts?
		if (m_ResidencyHandle.IsInitialized())
		{
			// D3DX12Residency::EndTrackingObject(GetParentDevice()->GetResidencyManager(), ResidencyHandle);
			// ResidencyHandle = {};
			m_pOwningManager->EndTrackingObject(&m_ResidencyHandle);
			::ZeroMemory(&m_ResidencyHandle, sizeof(m_ResidencyHandle));
		}

		SafeRelease(m_pRawHeap);
	}

	void ManagedD3D12Heap::BeginTrackingResidency()
	{
		ASSERT(m_pRawHeap);

		if (!m_ResidencyHandle.IsInitialized())
		{
			D3D12_HEAP_DESC HeapDesc = m_pRawHeap->GetDesc();

			m_ResidencyHandle.Initialize((ID3D12Pageable*)m_pRawHeap, HeapDesc.SizeInBytes, 0ull);
		}

		// TODO 0 : CustomD3D12Adapter or GPUNode
		m_pOwningManager->BeginTrackingObject(&m_ResidencyHandle);
	}

	void ManagedD3D12Heap::UpdateResidency(D3DX12Residency::ResidencySet* pResidencySet)
	{
		ASSERT(pResidencySet);
		
		if (!m_ResidencyHandle.IsInitialized())
		{
			BeginTrackingResidency();
		}
		
		pResidencySet->Insert(&m_ResidencyHandle);
	}
}