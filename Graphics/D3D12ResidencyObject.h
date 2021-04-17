#pragma once
#include "d3dx12Residency.h"

class ID3D12ResidencyObject
{
public:
	// ResidencyManager 때문에 어느정도 설계잡히면 그 때부터 상속 집어넣을 예정.
	// 아직은 아무것도 상속 안함.
	explicit ID3D12ResidencyObject(D3DX12Residency::ResidencyManager* pManager)
		:
		m_pOwningManager(pManager)
	{
		ASSERT(pManager);
	}

	virtual ~ID3D12ResidencyObject()
	{
		if (m_ResidencyHandle.IsInitialized())
		{
			m_pOwningManager->EndTrackingObject(&m_ResidencyHandle);
			::ZeroMemory(&m_ResidencyHandle, sizeof(m_ResidencyHandle));
		}
	}

	virtual void BeginTrackingResidency()                                      = 0;
	virtual void UpdateResidency(D3DX12Residency::ResidencySet* pResidencySet) = 0;

protected:
	D3DX12Residency::ResidencyManager* const m_pOwningManager;
	D3DX12Residency::ManagedObject           m_ResidencyHandle;
};