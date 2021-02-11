#include "stdafx.h"
#include "CustomCommandQALPool.h"

namespace custom
{
	CustomCommandQueue::CustomCommandQueue()
	{

	}
	CustomCommandQueue::CustomCommandQueue(ID3D12Device* pDevice, CustomCommandQALPool* pPool, D3D12_COMMAND_LIST_TYPE type)
	{

	}

	HRESULT CustomCommandQueue::Create(ID3D12Device* pDevice, CustomCommandQALPool* pPool, D3D12_COMMAND_LIST_TYPE type)
	{

	}

	void CustomCommandQueue::PairingFenceOff(ID3D12Fence* pFence)
	{

	}
	D3D12_COMMAND_LIST_TYPE CustomCommandQueue::GetType()
	{
		ASSERT(m_pCommandQueue);

		return m_Type;
	}
}

namespace custom
{
	CustomCommandAllocator::CustomCommandAllocator()
		: 
		m_Type(D3D12_COMMAND_LIST_TYPE_DIRECT),
		m_pDevice(nullptr),
		m_pOwningPool(nullptr),
		m_pAllocator(nullptr)
	{
	}
	CustomCommandAllocator::CustomCommandAllocator(ID3D12Device* pDevice, CustomCommandQALPool* pPool, D3D12_COMMAND_LIST_TYPE type)
		:
		m_Type(type),
		m_pDevice(pDevice),
		m_pOwningPool(pPool),
		m_pAllocator(nullptr)
	{
		ASSERT(pDevice);
		Create(pDevice, pPool, type);
	}

	CustomCommandAllocator::~CustomCommandAllocator()
	{
	}

	HRESULT CustomCommandAllocator::Create(ID3D12Device* pDevice, CustomCommandQALPool* pPool, D3D12_COMMAND_LIST_TYPE type)
	{
		ASSERT(pDevice);
		ASSERT(pPool);
		ASSERT(m_pAllocator == nullptr);

		m_pDevice = pDevice;
		m_pOwningPool = pPool;

		HRESULT Result;
		ASSERT_HR(Result = pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&m_pAllocator)));
		return Result;
	}
	HRESULT CustomCommandAllocator::Reset()
	{
		return m_pAllocator->Reset();
	}

	HRESULT CustomCommandAllocator::GetDevice(ID3D12Device** ppDevice)
	{
		ASSERT(*ppDevice == nullptr);

		HRESULT Result = m_pAllocator->GetDevice(IID_PPV_ARGS(ppDevice));
		ASSERT_HR(Result);

		return Result;
	}

	ID3D12CommandAllocator* CustomCommandAllocator::GetCommandAllocator()
	{
		return m_pAllocator;
	}
	D3D12_COMMAND_LIST_TYPE CustomCommandAllocator::GetType()
	{
		return m_Type;
	}
}

namespace custom
{
	CustomGraphicsCommandList::CustomGraphicsCommandList()
		: 
		m_pDevice(nullptr),
		m_pCommandQueue(nullptr),
		m_pQueueFence(nullptr),
		m_pOwningPool(nullptr),
		m_pPartnerAllocator(nullptr),
		m_pGraphicsCommandList(nullptr),
		m_bClosed(false),
		m_LastFenceValue(0)
	{
	}
	CustomGraphicsCommandList::CustomGraphicsCommandList
	(
		ID3D12Device*           pDevice,
		ID3D12CommandQueue*     pCommandQueue,
		CustomCommandQALPool*   pPool,
		CustomFence*            pCustomFence,
		CustomCommandAllocator* pAllocator,
		uint64_t                LastFenceValue
	)
		:
		m_pDevice(pDevice),
		m_pCommandQueue(pCommandQueue),
		m_pQueueFence(pCustomFence),
		m_pOwningPool(pPool),
		m_pPartnerAllocator(pAllocator),
		m_pGraphicsCommandList(nullptr),
		m_bClosed(false),
		m_LastFenceValue(LastFenceValue)
	{
		Create(pDevice, pAllocator);
	}
	CustomGraphicsCommandList::~CustomGraphicsCommandList()
	{
	}
	void CustomGraphicsCommandList::Create
	(
		ID3D12Device*           pDevice,
		CustomCommandAllocator* pAllocator
	)
	{
		ASSERT(pDevice);
		ASSERT(pAllocator);

		m_pDevice           = pDevice;
		m_pPartnerAllocator = pAllocator;

		ASSERT_HR(m_pDevice->CreateCommandList(1/*Single GPU*/, pAllocator->GetType(), pAllocator->GetCommandAllocator(), nullptr, IID_PPV_ARGS(&m_pGraphicsCommandList)));
		ASSERT(m_pGraphicsCommandList);
	}

	HRESULT CustomGraphicsCommandList::Reset(CustomCommandAllocator* pAllocator)
	{
		ID3D12CommandAllocator* pCommandAllocator = pAllocator->GetCommandAllocator();
		ASSERT(pCommandAllocator);
		HRESULT Result;
		ASSERT_HR(Result = m_pGraphicsCommandList->Reset(pCommandAllocator, nullptr));

		m_bClosed = false;

		return Result;
	}
	HRESULT CustomGraphicsCommandList::Close()
	{
		ASSERT(m_bClosed);

		HRESULT Result;
		ASSERT_HR(Result = m_pGraphicsCommandList->Close());
		m_bClosed = true;

		return Result;
	}

	HRESULT CustomGraphicsCommandList::GetDevice(ID3D12Device** ppDevice)
	{
		ASSERT(*ppDevice == nullptr);

		HRESULT Result = m_pGraphicsCommandList->GetDevice(IID_PPV_ARGS(ppDevice));
		ASSERT_HR(Result);

		return Result;
	}

	UINT CustomGraphicsCommandList::GetPriority() // Desc받아서
	{
		ASSERT(m_pCommandQueue);

		return m_pCommandQueue->GetDesc().Priority;
	}
	UINT CustomGraphicsCommandList::GetNodeMask() // Desc받아서
	{
		ASSERT(m_pCommandQueue);

		return m_pCommandQueue->GetDesc().NodeMask;
	}
	D3D12_COMMAND_LIST_TYPE CustomGraphicsCommandList::GetType() // Desc받아서
	{
		return m_pGraphicsCommandList->GetType();
	}

	uint64_t CustomGraphicsCommandList::GetLastFenceValue()
	{
		return m_LastFenceValue;
	}

	bool CustomGraphicsCommandList::GetbClosed()
	{
		return m_bClosed;
	}
}

namespace custom
{
	CustomCommandQALPool::CustomCommandQALPool()
		:
		m_pDevice(nullptr),
		m_Type(D3D12_COMMAND_LIST_TYPE_DIRECT),
		m_NumAvailableCommandQueues(0), 
		m_NumAvaiableCommandAllocators(0),
		m_NumAvaiableCommandLists(0)
	{
	}
	CustomCommandQALPool::CustomCommandQALPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
		:
		m_NumAvailableCommandQueues(0),
		m_NumAvaiableCommandAllocators(0),
		m_NumAvaiableCommandLists(0)
	{
		Create(pDevice, type);
	}

	CustomCommandQALPool::~CustomCommandQALPool()
	{
		Shutdown();
	}

	void CustomCommandQALPool::Create(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
	{
		ASSERT(pDevice);
		ASSERT
		(
			(type == D3D12_COMMAND_LIST_TYPE_DIRECT) ||
			(type == D3D12_COMMAND_LIST_TYPE_COPY) ||
			(type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
		);

		m_pDevice = pDevice;
		m_Type    = type;
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

		std::lock_guard<std::mutex> lockGuard(m_CommandQueueMutex);

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
			desc.Type = m_Type;
			desc.NodeMask = 1;
			desc.Priority = priority;

			ASSERT_HR(m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pCommandQueue)));

#ifdef _DEBUG
			pCommandQueue->SetName(StringToWString("CustomCommandQALPOOL #" + std::to_string(m_AllCommandQueues.size())).c_str());
#endif

			m_AllCommandQueues.push_back(pCommandQueue);
		}

		ASSERT(pCommandQueue);
		return pCommandQueue;
	}

	void CustomCommandQALPool::RequestCommandAllocatorsLists(size_t NumPairs, ID3D12CommandAllocator*** pppCommandAllocators, ID3D12GraphicsCommandList*** pppLists)
	{

		ASSERT(NumPairs);
		ASSERT(pppCommandAllocators);
		ASSERT(pppLists);

		ID3D12CommandAllocator**    pAllocatorArray = *pppCommandAllocators;
		ID3D12GraphicsCommandList** pListArray      = *pppLists;
		{
			std::lock_guard<std::mutex> lockGuard(m_AllocatorMutex);

			size_t AllocatorIndex = 0;
			size_t AllocatorNewIndex = 0;

			while (!m_AvailableCommandAllocators.empty())
			{
				pAllocatorArray[AllocatorIndex++] = m_AvailableCommandAllocators.front();
				m_AvailableCommandAllocators.pop_front();
				--m_NumAvaiableCommandAllocators;
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
			}
		}

		{
			std::lock_guard<std::mutex> lockGuard(m_ListMutex);

			size_t ListIndex = 0;
			size_t ListNewIndex = 0;

			while (!m_AvailableCommandLists.empty())
			{
				pListArray[ListIndex] = m_AvailableCommandLists.front();
				pListArray[ListIndex]->Reset(pAllocatorArray[ListIndex], nullptr);

				m_AvailableCommandLists.pop_front();
				++ListIndex;
				--m_NumAvaiableCommandLists;
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

			if ((1 < NumPairs) && NumPairs != ListNewIndex)
			{
				ID3D12Device* pDevice1;
				ID3D12Device* pDevice2;

				pAllocatorArray[0]->GetDevice(IID_PPV_ARGS(&pDevice1));
				pAllocatorArray[ListNewIndex]->GetDevice(IID_PPV_ARGS(&pDevice2));

				ASSERT(pDevice1 == pDevice2);
			}
		}
	}
}