#pragma once

namespace custom
{
	struct SmallThreadCoreInterfaces
	{
		ID3D12CommandQueue*     pPublicCommandQueue;
		ID3D12Fence*            pPublicFence;
		ID3D12CommandAllocator* pPrivateCommandAllocator;
		ID3D12CommandList*      pPrivateCommandList;

		size_t ArrayIndex;
		size_t ArrayMaxCount;
	};
	struct BigThreadCoreInterfaces
	{
		ID3D12CommandQueue*      pCommandQueue;
		ID3D12CommandAllocator** pCommandAllocators;
		ID3D12CommandList**      pCommandList;
		ID3D12Fence*             pFence; // Sync with another Big Threads.

		SmallThreadCoreInterfaces* pSmallThreadContext;

		size_t NumPairs; // Allocators and CommandLists.
	};
	

	class CustomCommandQueue
	{
	public:
		CustomCommandQueue();
		~CustomCommandQueue();

		void Create(ID3D12Device* pDevice);
		HRESULT AllocateBigThreadThings(size_t NumPairs);

	private:
		ID3D12Device* m_pDevice; // = device::g_pDevice;
	};

}
