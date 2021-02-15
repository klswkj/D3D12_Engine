#pragma once
#include "CustomCommandQALPool.h"
#include "CustomFence.h"

namespace custom
{
	// 개념 : 
	// 1개의 GPUWorkSubmission은
	// 한개의 CommandQueue에 여러개의 WorkThread가 붙어서 따로따로 CommandList에 붙어서,
	// CommandList를 채워서, CommandQueue를 최대한 빨리 GPU로 전송시키는게 목적.
	// Thread의 수는 PhysicalCore개수 만큼.

	// FD3D12CommandListManager에 대응.

	enum class EFiberPriority // Not used yet.
	{
		Priority_Normal,       // THREAD_PRIORITY_NORMAL 0
		Priority_Above_Normal, // THREAD_PRIORITY_ABOVE_NORMAL 1
		Priority_Highest       // THREAD_PRIORITY_HIGHEST 2
	};

	class TaskFiber
	{
		friend class TaskFiberManager;
	public:
		explicit TaskFiber(CustomCommandQALPool* pPool, ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type);
		~TaskFiber();
		D3D12_COMMAND_LIST_TYPE GetCommandType()  const { return m_Type; }
		ID3D12CommandQueue*     GetCommandQueue() const { return m_pCommandQueue; }
		ID3D12Fence*            GetFence()        const { return m_CustomFence.GetFence(); }
		size_t          GetNumCommandAllocators() const { return m_CommandAllocators.size(); }
		size_t          GetNumCommandLists()      const { return m_CommandLists.size(); }
		ID3D12CommandAllocator*    GetCommandAllocator(size_t index) { if (index < m_CommandAllocators.size()) return m_CommandAllocators[index]; else return nullptr; }
		ID3D12GraphicsCommandList* GetCommandList(size_t index)      { if (index < m_CommandLists.size()) return m_CommandLists[index]; else return nullptr; }
		
		void SetCommandQueue(ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type);
		_Check_return_ bool ResetAllocatorLists(size_t numPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type);
		void PushbackAllocatorLists(size_t numPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type);
		void ReleaseCommandPair(size_t numPair);

		bool IsCompletedTask(uint64_t fenceValue = 0);

		HRESULT  CloseLists();
		HRESULT  Reset();
		uint64_t ExecuteCommandQueue();

	private:
		CustomCommandQALPool* m_pOwningPool;
		ID3D12CommandQueue*   m_pCommandQueue;
		CustomFence           m_CustomFence;
		std::vector<ID3D12CommandAllocator*>    m_CommandAllocators;
		std::vector<ID3D12GraphicsCommandList*> m_CommandLists;

		D3D12_COMMAND_LIST_TYPE m_Type;
	};
	
	class TaskFiberManager
	{
	public:
		explicit TaskFiberManager();
		explicit TaskFiberManager(ID3D12Device* pDevice);
		~TaskFiberManager() { Shutdown(); }

		void Create(ID3D12Device* pDevice);
		void Shutdown();

		TaskFiber* RequestWorkSubmission(size_t numPairALs, D3D12_COMMAND_LIST_TYPE type, uint64_t completedFenceValue = 0); // pseudo
		void Release(TaskFiber* pWorkSubmission);
		
		CustomCommandQALPool* GetQALPool(D3D12_COMMAND_LIST_TYPE type);

	private:
		void RequestCommandAllocatorsLists(D3D12_COMMAND_LIST_TYPE type, size_t numLists, ID3D12CommandAllocator*** p_pCommandAllocatorArray, ID3D12GraphicsCommandList*** p_pListArray);

	private:
		static TaskFiberManager* s_pTaskFiberManager;

	private:
		ID3D12Device* m_pDevice;

		// COMMAND_LIST_TYPE [DIRECT, COMPUTE, COPY]
		std::vector<TaskFiber*> m_ProcessingTask[3];
		CustomCommandQALPool m_QALPool[3];

		CRITICAL_SECTION m_CS; // Not used yet.
	};
}
