#pragma once

namespace custom
{
	struct SmallRenderContext
	{
		ID3D12CommandQueue*     pPublicCommandQueue;
		ID3D12Fence*            pPublicFence;
		ID3D12CommandAllocator* pPrivateCommandAllocator;
		ID3D12CommandList*      pPrivateCommandList;

		size_t ArrayIndex;    // pSmallThreadContext���� �迭 �ε���
		size_t ArrayMaxCount; // pSmallThreadContext�迭�� �ִ� ũ�� -> �迭�� �Ⱦ��� std::vector�� �� ���� �ְ�,
	};
	struct BigRenderContext
	{
		ID3D12CommandQueue*      pCommandQueue;
		ID3D12CommandAllocator** pCommandAllocators;
		ID3D12CommandList**      pCommandList;
		ID3D12Fence*             pFence; // Sync with another Big Threads.

		SmallRenderContext* pSmallThreadContexts;

		size_t NumPairs; // Allocators and CommandLists. 
		                 // -> �� Allocator, CommandList�� ���� ũ��(pSmallThreadContext[] ������)
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
	

	// 0. Pass�� ������ ���� ������ PassFlag(bool) -> AsyncPass����, SyncPass����
	//    �ƴϸ� Dynamic_cast�� �˾Ƴ�����(�̰Ŵ� �Լ��� �޾ƾ��ϴϱ� �������̰�)
	// 0-1. ���� �ڷκ��ʹ� Sync�� �ʿ��ؼ�, 
	//      GPU�� ������ �� ������, ���ƿͼ� Fence���� Ȯ���ؾ��ϴ� 
	//      AsyncPass, �� �ݴ��� Sync.
	// 0-2. RenderQueue�� �ִ� RenderQueuePass.
	//      �ݸ鿡, RenderQueue�� ����, Buffer(RenderTarget Texture)�� ��븸 ����ؼ�,
	//      �׸��� FullScreenPass.
	//      �׸��� ���� �E �ʿ䰡 �ֱ�� ������, ComputeShader���� ����ϴ� ComputePass??
	//      �� ��� �з��� ���߿� ����������� ���� �����ֱ� ���� �Լ��� �޾� �ö��� ���Ͽ� �з��ϴ°�...


	// 1. Synchronization(����ȭ)�� �ʿ��� Pass������
	//    �ϳ��� ID3D12CommandQueue�� �ʿ���.
	//    ex) ���� ��� 1�� Pass���� 4���н�(SSAOPass)����.
	// 1-1. 1���� �����Ϸ��� SSAOPass���� �ϳ� BigContext�� ����ߵǴ��� �˾ƾ���.
	//      -> �� Pass���� Async�ؾ��ϴ����� Flag(bool ��)���� Ȯ��.
	// 1-2. RenderQueuePass�̸�, RenderQueue�� Job�� ����� Ȯ��.
	//      MasterRenderGraph::m_ThreadJobScale�� ��ŭ Job�� ©�� SmallRenderContext�� �Ҵ� �������.
}
