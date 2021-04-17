#include "stdafx.h"
#include "GPUResource.h"
#include "ManagedD3D12Heap.h"

namespace custom
{
    D3DX12Residency::ManagedObject* GPUResource::GetResidencyHandle()
    {
        return m_pManagedHeap ? m_pManagedHeap->GetResidencyHandle() : &m_ResidencyHandle;
    }

    void GPUResource::BeginTrackingResidency()
    {
        if (!m_pManagedHeap) // Is Committed Resource?
		{
			{
				D3D12_HEAP_PROPERTIES HeapProperities = {};
				m_pResource->GetHeapProperties(&HeapProperities, nullptr);
				ASSERT((m_pManagedHeap == nullptr) && m_pResource && D3D12Util::IsCPUInaccessible(HeapProperities.Type));
			}

			if (!m_ResidencyHandle.IsInitialized())
			{
				ID3D12Device* pDevice = nullptr;
				ASSERT_HR(m_pResource->GetDevice(IID_PPV_ARGS(&pDevice)));

				D3D12_RESOURCE_DESC ResourceDesc = m_pResource->GetDesc();

				D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo = pDevice->GetResourceAllocationInfo(m_GPUVisibleNodeMask.NodeMask, 1, &ResourceDesc);
				pDevice->Release();

				m_ResidencyHandle.Initialize((ID3D12Pageable*)m_pResource, AllocationInfo.Alignment, 0ull);
			}

            m_pOwningManager->BeginTrackingObject(&m_ResidencyHandle);
        }
        else
        {
            m_pManagedHeap->BeginTrackingResidency();
        }
    }

    void GPUResource::UpdateResidency(D3DX12Residency::ResidencySet* pResidencySet)
    {
        ASSERT(pResidencySet);

        if (!m_pManagedHeap) // Is Committed Resource?
        {
            if (!m_ResidencyHandle.IsInitialized())
            {
                BeginTrackingResidency();
            }

            pResidencySet->Insert(&m_ResidencyHandle);
        }
        else
        {
            m_pManagedHeap->UpdateResidency(pResidencySet);
        }
    }
}