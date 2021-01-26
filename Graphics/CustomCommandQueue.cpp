#include "stdafx.h"
#include "CustomCommandQueue.h"

namespace custom
{
	void CustomCommandQueue::Create(ID3D12Device* pDevice)
	{
		ASSERT(pDevice != nullptr);
		m_pDevice = pDevice;
	}

	HRESULT CustomCommandQueue::AllocateThreadContext(size_t NumPairs)
	{
		ASSERT(m_pDevice != nullptr);


	}
}