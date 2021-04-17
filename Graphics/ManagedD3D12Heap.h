#pragma once
#include "CommandContext.h"

namespace custom
{
	// Also this will be BackingHeap of BuddyAllocator.
	class ManagedD3D12Heap // : public FD3D12RefCount, public FD3D12DeviceChild, public FD3D12MultiNodeGPUObject
	{
	public:
		// ManagedD3D12Heap(FD3D12Device* Parent, FRHIGPUMask VisibleNodes);
		explicit ManagedD3D12Heap(ID3D12Heap* pRawHeap, D3DX12Residency::ResidencyManager* const pResidencyManager)
			:
			m_pRawHeap(pRawHeap),
			m_pOwningManager(pResidencyManager)
		{
			ASSERT(pRawHeap && pResidencyManager);
		}

		virtual ~ManagedD3D12Heap();

		void Destroy();

		inline ID3D12Heap* GetHeap() { return m_pRawHeap; }
		inline D3DX12Residency::ManagedObject* GetResidencyHandle() { return &m_ResidencyHandle; }

		// inline void SetHeap(ID3D12Heap* HeapIn) { m_pRawHeap = HeapIn; }

		void BeginTrackingResidency();
		void UpdateResidency(D3DX12Residency::ResidencySet* pResidencySet);

	protected:
		ID3D12Heap* m_pRawHeap;
		D3DX12Residency::ResidencyManager* const m_pOwningManager;
		D3DX12Residency::ManagedObject           m_ResidencyHandle;
		// UINT m_ResourceSize      = m_pRawHeap->GetDesc().Properties.SizeInBytes
		// UINT GPUCreationNodeMask = m_pRawHeap->GetDesc().Properties.CreationNode
		// UINT GPUVisibleNodeMask  = m_pRawHeap->GetDesc().Properties.VisibleNode
	};
}

// Sizer<custom::ManagedD3D12Heap> SizerManagedD3D12Heap;