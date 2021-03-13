#pragma once


namespace custom
{
	class CustomCommandQALPool;
	class CustomFence;

	class CustomCommandQALPool
	{
	public:
		explicit CustomCommandQALPool();
		explicit CustomCommandQALPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);
		~CustomCommandQALPool();

		void Create(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);
		void Shutdown();

		ID3D12CommandQueue* RequestCommandQueue(UINT priority = 0);
		void RequestCommandAllocatorsLists(size_t NumLists, ID3D12CommandAllocator*** p_pCommandAllocatorArray, ID3D12GraphicsCommandList*** p_pListArray);

		std::deque<ID3D12CommandQueue*       >* GetAvailableCommandQueues()     { return &m_AvailableCommandQueues; }
		std::deque<ID3D12CommandAllocator*   >* GetAvailableCommandAllocators() { return &m_AvailableCommandAllocators; }
		std::deque<ID3D12GraphicsCommandList*>* GetAvailableCommandLists()      { return &m_AvailableCommandLists; }

		void ReleaseCommandQueue(ID3D12CommandQueue* pCommandQueue);

		void ReleaseCommandAllocator (ID3D12CommandAllocator*  pCommandAllocator);
		void ReleaseCommandAllocators(ID3D12CommandAllocator** pCommandAllocators, uint64_t allocatorSize);

		void ReleaseCommandList (ID3D12GraphicsCommandList*  pCommandList);
		void ReleaseCommandLists(ID3D12GraphicsCommandList** pCommandLists, uint64_t listSize);

	private:
		ID3D12Device*           m_pDevice;
		D3D12_COMMAND_LIST_TYPE m_Type;

		std::vector<ID3D12CommandQueue*> m_AllCommandQueues;
		std::deque<ID3D12CommandQueue*>  m_AvailableCommandQueues;

		std::vector<ID3D12CommandAllocator*> m_AllCommandAllocators;
		std::deque<ID3D12CommandAllocator*>  m_AvailableCommandAllocators;

		std::vector<ID3D12GraphicsCommandList*> m_AllCommandLists;
		std::deque<ID3D12GraphicsCommandList*>  m_AvailableCommandLists;

		size_t m_NumAvailableCommandQueues;
		size_t m_NumAvailableCommandAllocators;
		size_t m_NumAvailableCommandLists;

		CRITICAL_SECTION m_CommandQueueCS;
		CRITICAL_SECTION m_CommandAllocatorCS;
		CRITICAL_SECTION m_CommandListCS;
	};
}