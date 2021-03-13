#include "stdafx.h"
#include "CustomCommandQALPool.h"

namespace custom
{
	CustomCommandQALPool::CustomCommandQALPool()
		:
		m_pDevice(nullptr),
		m_Type(D3D12_COMMAND_LIST_TYPE_DIRECT),
		m_NumAvailableCommandQueues(0ul), 
		m_NumAvailableCommandAllocators(0ul),
		m_NumAvailableCommandLists(0ul)
	{
		InitializeCriticalSection(&m_CommandQueueCS);
		InitializeCriticalSection(&m_CommandAllocatorCS);
		InitializeCriticalSection(&m_CommandListCS);
	}
	CustomCommandQALPool::CustomCommandQALPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
		:
		m_CommandQueueCS({}), 
		m_CommandAllocatorCS({}),
		m_CommandListCS({}),
		m_NumAvailableCommandQueues(0ul),
		m_NumAvailableCommandAllocators(0ul),
		m_NumAvailableCommandLists(0ul)
	{
		Create(pDevice, type);
		InitializeCriticalSection(&m_CommandQueueCS);
		InitializeCriticalSection(&m_CommandAllocatorCS);
		InitializeCriticalSection(&m_CommandListCS);
	}

	void CustomCommandQALPool::Create(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
	{
		ASSERT
		(
			pDevice &&
			(type == D3D12_COMMAND_LIST_TYPE_DIRECT) ||
			(type == D3D12_COMMAND_LIST_TYPE_COPY)   ||
			(type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
		);

		m_pDevice = pDevice;
		m_Type    = type;
	}

	CustomCommandQALPool::~CustomCommandQALPool()
	{
		Shutdown();
	}
	void CustomCommandQALPool::Shutdown()
	{
		for (auto& e : m_AllCommandQueues)
		{
			SafeRelease(e);
		}

		for (auto& e : m_AllCommandAllocators)
		{
			SafeRelease(e);
		}

		for (auto& e : m_AllCommandLists)
		{
			SafeRelease(e);
		}

		DeleteCriticalSection(&m_CommandQueueCS);
		DeleteCriticalSection(&m_CommandAllocatorCS);
		DeleteCriticalSection(&m_CommandListCS);
	}

	// TODO 1 : 
	// 여기 나중에 원래 있던 CommandQueue랑 통폐합해서, 
	// Priority도 담아서 Capsulation하자.
	// 나중 되면 Priority 무조건 필요함.
	// 나중에 멀티엔진 되면 NodeMask도 받아야함.

	// -> Capsulation된거 지금 만들어야될듯.
	// *1. GPU에 제출되었는지
	// 2. priority? -> 함수로
	// 3. m_type -> 함수로
	// *4. 2,3 처럼 GetDesc로 할 수 있는거는 뺴고, 내부에서 GetDesc를 해서 Return하도록.
	// *5. 제출된 FenceValue

	// TODO 3 : Priority
	ID3D12CommandQueue* CustomCommandQALPool::RequestCommandQueue(UINT priority/*= 0*/)
	{
		ID3D12CommandQueue* pCommandQueue = nullptr;

		EnterCriticalSection(&m_CommandQueueCS);

		if (!m_AvailableCommandQueues.empty())
		{
			pCommandQueue = m_AvailableCommandQueues.front();

			m_AvailableCommandQueues.pop_front();
			--m_NumAvailableCommandQueues;
		}
		else
		{
			D3D12_COMMAND_QUEUE_DESC desc;
			ZeroMemory(&desc, sizeof(D3D12_COMMAND_QUEUE_DESC));
			desc.Type     = m_Type;
			desc.NodeMask = 1;
			desc.Priority = priority;

			ASSERT_HR(m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pCommandQueue)));

#ifdef _DEBUG
			pCommandQueue->SetName(StringToWString("CustomCommandQALPOOL #" + std::to_string(m_AllCommandQueues.size())).c_str());
#endif
			m_AllCommandQueues.push_back(pCommandQueue);
		}

		LeaveCriticalSection(&m_CommandQueueCS);

		ASSERT(pCommandQueue);
		return pCommandQueue;
	}

	void CustomCommandQALPool::RequestCommandAllocatorsLists(size_t NumPairs, ID3D12CommandAllocator*** p_pCommandAllocators, ID3D12GraphicsCommandList*** p_pLists)
	{
		ASSERT(NumPairs && p_pCommandAllocators && p_pLists);

		ID3D12CommandAllocator**    pAllocatorArray = *p_pCommandAllocators;
		ID3D12GraphicsCommandList** pListArray      = *p_pLists;
		{
			EnterCriticalSection(&m_CommandAllocatorCS);

			size_t AllocatorIndex = 0;
			size_t AllocatorNewIndex = 0;

			while (!m_AvailableCommandAllocators.empty() && (AllocatorIndex < NumPairs))
			{
				pAllocatorArray[AllocatorIndex++] = m_AvailableCommandAllocators.front();
				m_AvailableCommandAllocators.pop_front();
				--m_NumAvailableCommandAllocators;
			}

			AllocatorNewIndex = AllocatorIndex;
			m_AllCommandAllocators.reserve(m_AllCommandAllocators.capacity() + NumPairs - AllocatorIndex);

			while (AllocatorIndex < NumPairs)
			{
				ID3D12CommandAllocator* pAllocator = nullptr;
				ASSERT_HR(m_pDevice->CreateCommandAllocator(m_Type, IID_PPV_ARGS(&pAllocator)));

#if defined(_DEBUG)
				pAllocator->SetName(StringToWString("CustomCommandQALPOOL #" + std::to_string(m_AllCommandAllocators.size())).c_str());
#endif
				m_AllCommandAllocators.push_back(pAllocator);
				pAllocatorArray[AllocatorIndex++] = pAllocator;
			}

			for (size_t i = 0; i < NumPairs; ++i)
			{
				ASSERT_HR(pAllocatorArray[i]->Reset());
			}

			if ((1 < NumPairs) && NumPairs != AllocatorNewIndex)
			{
				ID3D12Device* pDevice1;
				ID3D12Device* pDevice2;

				pAllocatorArray[0]->GetDevice(IID_PPV_ARGS(&pDevice1));
				pAllocatorArray[AllocatorNewIndex]->GetDevice(IID_PPV_ARGS(&pDevice2));

				ASSERT(pDevice1 == pDevice2);

				SafeRelease(pDevice1);
				SafeRelease(pDevice2);
			}

			LeaveCriticalSection(&m_CommandAllocatorCS);
		}

		{
			EnterCriticalSection(&m_CommandListCS);

			size_t ListIndex = 0;
			size_t ListNewIndex = 0;

			while (!m_AvailableCommandLists.empty() && (ListIndex < NumPairs))
			{
				pListArray[ListIndex] = m_AvailableCommandLists.front();
				pListArray[ListIndex]->Reset(pAllocatorArray[ListIndex], nullptr);

				m_AvailableCommandLists.pop_front();
				++ListIndex;
				--m_NumAvailableCommandLists;
			}

			ListNewIndex = ListIndex;
			m_AllCommandLists.reserve(m_AllCommandLists.capacity() + NumPairs - ListIndex);

			while (ListIndex < NumPairs)
			{
				ID3D12GraphicsCommandList* pList = nullptr;
				ASSERT_HR(m_pDevice->CreateCommandList(1, m_Type, pAllocatorArray[ListIndex], nullptr, IID_PPV_ARGS(&pList)));
				pListArray[ListIndex]->Reset(pAllocatorArray[ListIndex], nullptr);

#if defined(_DEBUG)
				pList->SetName(StringToWString("CustomCommandQALPOOL #" + std::to_string(m_AllCommandLists.size())).c_str());
#endif
				m_AllCommandLists.push_back(pList);
				pListArray[ListIndex++] = pList;
			}

			if ((1 < NumPairs) && (NumPairs != ListNewIndex))
			{
				ID3D12Device* pDevice1;
				ID3D12Device* pDevice2;

				pAllocatorArray[0]->GetDevice(IID_PPV_ARGS(&pDevice1));
				pAllocatorArray[ListNewIndex]->GetDevice(IID_PPV_ARGS(&pDevice2));

				ASSERT(pDevice1 == pDevice2);

				pDevice1->Release();
				pDevice2->Release();
			}

			LeaveCriticalSection(&m_CommandListCS);
		}
	}

	void CustomCommandQALPool::ReleaseCommandQueue(ID3D12CommandQueue* pCommandQueue)
	{
		EnterCriticalSection(&m_CommandQueueCS);
		m_AvailableCommandQueues.push_back(pCommandQueue);
		++m_NumAvailableCommandQueues;
		LeaveCriticalSection(&m_CommandQueueCS);
	}
	void CustomCommandQALPool::ReleaseCommandAllocator(ID3D12CommandAllocator* pCommandAllocator)
	{
		EnterCriticalSection(&m_CommandAllocatorCS);
		m_AvailableCommandAllocators.push_back(pCommandAllocator);
		++m_NumAvailableCommandAllocators;
		LeaveCriticalSection(&m_CommandAllocatorCS);
	}
	void CustomCommandQALPool::ReleaseCommandList(ID3D12GraphicsCommandList* pCommandList)
	{
		EnterCriticalSection(&m_CommandListCS);
		m_AvailableCommandLists.push_back(pCommandList);
		++m_NumAvailableCommandLists;
		LeaveCriticalSection(&m_CommandListCS);
	}

	void CustomCommandQALPool::ReleaseCommandAllocators(ID3D12CommandAllocator** pCommandAllocators, uint64_t allocatorSize)
	{
		EnterCriticalSection(&m_CommandAllocatorCS);
		for (size_t i = 0; i < allocatorSize; ++i)
		{
			ASSERT_HR(pCommandAllocators[i]->Reset());
			m_AvailableCommandAllocators.push_back(pCommandAllocators[i]);
		}
		m_NumAvailableCommandAllocators += allocatorSize;
		LeaveCriticalSection(&m_CommandAllocatorCS);
	}
	void CustomCommandQALPool::ReleaseCommandLists(ID3D12GraphicsCommandList** pCommandLists, uint64_t listSize)
	{
		EnterCriticalSection(&m_CommandListCS);
		for (size_t i = 0; i < listSize; ++i)
		{
			pCommandLists[i]->Reset(nullptr, nullptr);
			m_AvailableCommandLists.push_back(pCommandLists[i]);
		}
		m_NumAvailableCommandLists += listSize;
		LeaveCriticalSection(&m_CommandListCS);
	}
}