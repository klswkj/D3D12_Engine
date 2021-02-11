#include "stdafx.h"
#include "GPUWorkManager.h"

namespace custom
{
	TaskFiber::TaskFiber(ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type, ID3D12Fence* pFence, uint64_t CPUFenceValue)
		:
		m_Type(type),
		m_CustomFence(type)
	{
		ASSERT(pCommandQueue);
		ASSERT(pFence);
		ASSERT
		(
			type == D3D12_COMMAND_LIST_TYPE_DIRECT ||
			type == D3D12_COMMAND_LIST_TYPE_COPY   ||
			type == D3D12_COMMAND_LIST_TYPE_COMPUTE
		);
		ASSERT((D3D12_COMMAND_LIST_TYPE)(CPUFenceValue >> 56) == type);

		m_pCommandQueue = pCommandQueue;
		m_CustomFence.Reset(pFence, m_Type, CPUFenceValue);
	}

	bool TaskFiber::IsCompletedTask(uint64_t fenceValue = 0)
	{
		return m_CustomFence.IsFenceComplete(fenceValue);
	}

	void TaskFiber::SetCommandQueue(ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type)
	{
		ASSERT(pCommandQueue);

		D3D12_COMMAND_QUEUE_DESC desc = pCommandQueue->GetDesc();
		ASSERT(desc.Type == type);

		m_pCommandQueue = pCommandQueue;
		m_CustomFence.SetCommandQueue(pCommandQueue);
	}
	void TaskFiber::SetFence(ID3D12Fence* pFence, D3D12_COMMAND_LIST_TYPE type, uint64_t CPUSideNextFenceValue)
	{
		ASSERT(pFence);
		ASSERT(m_Type == type);

		m_CustomFence.Reset(pFence, type, CPUSideNextFenceValue);
	}
	bool TaskFiber::ResetAllocatorLists(size_t NumPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type)
	{
		bool bSafe = IsCompletedTask();

		if (bSafe)
		{
			m_CommandAllocators.clear();
			m_CommandLists.clear();
			m_CommandAllocators.reserve(NumPair);
			m_CommandLists.reserve(NumPair);

			m_CommandAllocators.insert(m_CommandAllocators.end(), pAllocatorArray, pAllocatorArray + NumPair);
			m_CommandLists.insert(m_CommandLists.end(), pListArray, pListArray + NumPair);
		}

		return bSafe;
	}
	void TaskFiber::PushbackAllocatorLists(size_t NumPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type)
	{
		m_CommandAllocators.reserve(m_CommandAllocators.size() + NumPair);
		m_CommandLists.reserve(m_CommandLists.size() + NumPair);

		m_CommandAllocators.insert(m_CommandAllocators.end(), pAllocatorArray, pAllocatorArray + NumPair);
		m_CommandLists.insert(m_CommandLists.end(), pListArray, pListArray + NumPair);
	}

	HRESULT TaskFiber::CloseLists()
	{
		for (auto& e : m_CommandLists)
		{
			ASSERT_HR(e->Close());
		}
	}

	uint64_t TaskFiber::ExecuteCommandQueue()
	{
		m_pCommandQueue->ExecuteCommandLists(m_CommandLists.size(), (ID3D12CommandList* const*)m_CommandLists.data());
	}

	HRESULT TaskFiber::Reset()
	{
		ASSERT(m_CommandAllocators.size() == m_CommandLists.size());

		size_t ElementSize = m_CommandAllocators.size();

		for (size_t i = 0; i < ElementSize; ++i)
		{
			ASSERT(m_CommandAllocators[i]->Reset());
			ASSERT(m_CommandLists[i]->Reset(m_CommandAllocators[i], nullptr));
		}
	}
}

namespace custom
{
	TaskFiberManager::TaskFiberManager(ID3D12Device* pDevice)
		:
		m_pDevice(pDevice)
	{
		ASSERT(pDevice);
		m_QALPool[0].Create(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_QALPool[1].Create(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		m_QALPool[2].Create(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
	}
	TaskFiberManager::~TaskFiberManager()
	{
		// Destoryer Calls
		// m_DirectQALPool.Shutdown();
		// m_CopyQALPool.Shutdown();
		// m_ComputeQALPool.Shutdown();
		// m_CustomFencePool.Shutdown();
	}

	// �ٵ� Allocate���ְ�, 
	// CommandQueue��, Fence���ݾ�
	// �ٵ� ���� �޾Ҵµ�, �װ� Fence �ȳ��������� Ȯ���ؾߵ�.
	TaskFiber* TaskFiberManager::RequestWorkSubmission(size_t numPairs, D3D12_COMMAND_LIST_TYPE type, uint64_t completedFenceValue)
	{
		ASSERT
		(
			(type == D3D12_COMMAND_LIST_TYPE_DIRECT) ||
			(type == D3D12_COMMAND_LIST_TYPE_COPY)   ||
			(type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
		);

		TaskFiber* pTaskFiber = nullptr;
		std::vector<TaskFiber>& ProcessingTasks = m_ProcessingTask[(size_t)type / 2];
		CustomCommandQALPool& Pool = m_QALPool[(size_t)type / 2];
		
		// �ϴ� ProcessingTask�߿� ������ �ִ��� Ȯ��.
		// ������ ������, �� �߿��� �䱸�� numPairs��ŭ �����ְ�, �������� Pool�� ��ȯ
		for (auto& e : ProcessingTasks)
		{
			if (e.IsCompletedTask())
			{

			}
		}

	}

	void TaskFiberManager::Release(TaskFiber* pWorkSubmission)
	{

	}

	// ���� �ǽ��� �帧.
	// 1. RequestWorkSubmission�� �ϸ� �����͸� �Ѱ��ְ� �ʹ�.
	// 2. �ٵ� ���⼭ ������ �����͸� �ָ� std::vector�� Ȯ��Ǹ�,
	// 3. std::vector�� �����̳�(�迭)�� �����ؼ� �ű��
	// �����̳��� ������ �ּҴ� ���ϰ� �Ǵµ�,
	// ������ CommandQueue,Fence,CommandAllocator,CommandList�� �����ͺ�����
	// �Ⱥ��ϴµ� bool������ �ٲ�ԵǼ� ���߿� Ȯ��Ǹ� �������� ���ϰԵ�

	// �׷��� std::vector<TaskFiber*>���� �ٲٸ� �Ǳ��ϴµ�, 
	// �޸� ����ȭ�� ��������... �׷��� MemoryPool�� �ٲٰ�ʹ�.
}