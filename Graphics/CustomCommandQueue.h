#pragma once

namespace custom
{
	struct SmallRenderContext
	{
		ID3D12CommandQueue*     pPublicCommandQueue;
		ID3D12Fence*            pPublicFence;
		ID3D12CommandAllocator* pPrivateCommandAllocator;
		ID3D12CommandList*      pPrivateCommandList;

		size_t ArrayIndex;    // pSmallThreadContext내의 배열 인덱스
		size_t ArrayMaxCount; // pSmallThreadContext배열의 최대 크기 -> 배열을 안쓰고 std::vector를 쓸 수도 있고,
	};
	struct BigRenderContext
	{
		ID3D12CommandQueue*      pCommandQueue;
		ID3D12CommandAllocator** pCommandAllocators;
		ID3D12CommandList**      pCommandList;
		ID3D12Fence*             pFence; // Sync with another Big Threads.

		SmallRenderContext* pSmallThreadContexts;

		size_t NumPairs; // Allocators and CommandLists. 
		                 // -> 총 Allocator, CommandList의 쌍의 크기(pSmallThreadContext[] 내에서)
	};
	
	class CustomCommandQueue
	{
	public:
		CustomCommandQueue() {} // pseudo
		~CustomCommandQueue() {} // pseudo

		void Create(ID3D12Device* pDevice);
		HRESULT AllocateBigContext(size_t NumPairs); // pseudo

	private:
		ID3D12Device* m_pDevice; // = device::g_pDevice;
	};
	

	// 0. Pass의 종류를 각자 고유의 PassFlag(bool) -> AsyncPass인지, SyncPass인지
	//    아니면 Dynamic_cast로 알아내던가(이거는 함수를 받아야하니까 무조건이고)
	// 0-1. 여기 뒤로부터는 Sync가 필요해서, 
	//      GPU가 실행을 다 끝나고, 돌아와서 Fence값을 확인해야하는 
	//      AsyncPass, 그 반대인 Sync.
	// 0-2. RenderQueue가 있는 RenderQueuePass.
	//      반면에, RenderQueue가 없고, Buffer(RenderTarget Texture)를 사용만 사용해서,
	//      그리는 FullScreenPass.
	//      그리고 따로 뺼 필요가 있기야 싶지만, ComputeShader만을 사용하는 ComputePass??
	//      이 모든 분류는 나중에 스레드들한테 일을 던져주기 위한 함수를 받아 올때를 위하여 분류하는거...


	// 1. Synchronization(동기화)가 필요할 Pass까지가
	//    하나의 ID3D12CommandQueue가 필요함.
	//    ex) 나의 경우 1번 Pass부터 4번패스(SSAOPass)까지.
	// 1-1. 1번이 성립하려면 SSAOPass에서 하나 BigContext로 끊어야되는지 알아야함.
	//      -> 이 Pass에서 Async해야하는지는 Flag(bool 값)으로 확인.
	// 1-2. RenderQueuePass이면, RenderQueue의 Job이 몇개인지 확인.
	//      MasterRenderGraph::m_ThreadJobScale개 만큼 Job을 짤라서 SmallRenderContext를 할당 받으면됨.
}
