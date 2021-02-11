#pragma once


namespace custom
{
	class CustomCommandQALPool;
	class CustomFence;

	class CustomCommandQueue
	{
	public:
		explicit CustomCommandQueue();
		explicit CustomCommandQueue(ID3D12Device* pDevice, CustomCommandQALPool* pPool, D3D12_COMMAND_LIST_TYPE type);

		HRESULT Create(ID3D12Device* pDevice, CustomCommandQALPool* pPool, D3D12_COMMAND_LIST_TYPE type);

		void PairingFenceOff(ID3D12Fence* pFence);
		D3D12_COMMAND_LIST_TYPE GetType();

	private:
		// ID3D12Device* m_pDevice; -> GetDevice()·Î Ãæ´ç.
		ID3D12CommandQueue*   m_pCommandQueue;
		CustomCommandQALPool* m_pOwningPool;
		ID3D12Fence*          m_pFence;
		uint64_t              m_FenceValue;

		D3D12_COMMAND_LIST_TYPE m_Type;
		// bool 
	};

	class CustomCommandAllocator
	{
	public:
		explicit CustomCommandAllocator();
		explicit CustomCommandAllocator(ID3D12Device* pDevice, CustomCommandQALPool* pPool, D3D12_COMMAND_LIST_TYPE type);
		~CustomCommandAllocator();

		HRESULT Create(ID3D12Device* pDevice, CustomCommandQALPool* pPool, D3D12_COMMAND_LIST_TYPE type);
		HRESULT Reset();

		HRESULT GetDevice(ID3D12Device** ppDevice);
		ID3D12CommandAllocator* GetCommandAllocator();
		D3D12_COMMAND_LIST_TYPE GetType();

	private:
		D3D12_COMMAND_LIST_TYPE m_Type;
		ID3D12Device*           m_pDevice;
		CustomCommandQALPool*   m_pOwningPool;
		ID3D12CommandAllocator* m_pAllocator;
	};

	class CustomGraphicsCommandList
	{
	public:
		explicit CustomGraphicsCommandList();
		explicit CustomGraphicsCommandList
		(
			ID3D12Device*              pDevice, 
			ID3D12CommandQueue*        pCommandQueue,
			CustomCommandQALPool*      pPool,
			CustomFence*               pCustomFence,
			CustomCommandAllocator*    pAllocator,
			uint64_t                   LastFenceValue
		);
		~CustomGraphicsCommandList();

		void Create
		(
			ID3D12Device* pDevice,
			CustomCommandAllocator* pAllocator
		);

		HRESULT GetDevice(ID3D12Device** ppDevice);
		D3D12_COMMAND_LIST_TYPE GetType();
		UINT     GetPriority();
		UINT     GetNodeMask();
		uint64_t GetLastFenceValue();
		bool     GetbClosed();

		HRESULT Reset(CustomCommandAllocator* pAllocator);
		HRESULT Close();

	private:
		ID3D12Device*              m_pDevice;
		ID3D12CommandQueue*        m_pCommandQueue;
		CustomFence*               m_pQueueFence;
		CustomCommandQALPool*      m_pOwningPool;
		CustomCommandAllocator*    m_pPartnerAllocator;
		ID3D12GraphicsCommandList* m_pGraphicsCommandList;

		// const D3D12_COMMAND_LIST_TYPE m_Type;
		uint64_t m_LastFenceValue;
		bool     m_bClosed;
	};

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

	private:
		ID3D12Device*           m_pDevice;
		D3D12_COMMAND_LIST_TYPE m_Type;

		size_t m_NumAvailableCommandQueues;
		size_t m_NumAvaiableCommandAllocators;
		size_t m_NumAvaiableCommandLists;

		std::vector<ID3D12CommandQueue*> m_AllCommandQueues;
		std::deque<ID3D12CommandQueue*>  m_AvailableCommandQueues;

		std::vector<ID3D12CommandAllocator*> m_AllCommandAllocators;
		std::deque<ID3D12CommandAllocator*> m_AvailableCommandAllocators;

		std::vector<ID3D12GraphicsCommandList*> m_AllCommandLists;
		std::deque<ID3D12GraphicsCommandList*> m_AvailableCommandLists;

		std::mutex m_CommandQueueMutex;
		std::mutex m_AllocatorMutex;
		std::mutex m_ListMutex;
	};

}