#pragma once
#include "CustomCommandQALPool.h"
#include "CustomFence.h"

namespace custom
{
	// ���� : 
	// 1���� GPUWorkSubmission��
	// �Ѱ��� CommandQueue�� �������� WorkThread�� �پ ���ε��� CommandList�� �پ,
	// CommandList�� ä����, CommandQueue�� �ִ��� ���� GPU�� ���۽�Ű�°� ����.
	// Thread�� ���� PhysicalCore���� ��ŭ.

	// FD3D12CommandListManager�� ����.

	enum class EFiberPriority // Not used yet.
	{
		Priority_Low,
		Priority_Medium,
		Priority_High
	};

	class TaskFiber
	{
		friend class TaskFiberManager;
	public:
		explicit TaskFiber(ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type, ID3D12Fence* pFence, uint64_t CpuFenceValue);

		ID3D12CommandQueue* GetCommandQueue() { return m_pCommandQueue; }
		ID3D12Fence*        GetFence() { return m_CustomFence.GetFence(); }
		ID3D12CommandAllocator* GetCommandAllocator(size_t Index) { if (Index < m_CommandAllocators.size()) return m_CommandAllocators[Index]; else return nullptr; }
		ID3D12GraphicsCommandList* GetCommandList(size_t Index)   { if (Index < m_CommandLists.size()) return m_CommandLists[Index]; else return nullptr; }
		size_t GetNumCommandAllocators() const { return m_CommandAllocators.size(); }
		size_t GetNumCommandLists() const { return m_CommandLists.size(); }

		void SetCommandQueue(ID3D12CommandQueue* pCommandQueue, D3D12_COMMAND_LIST_TYPE type);
		void SetFence(ID3D12Fence* pFence, D3D12_COMMAND_LIST_TYPE type, uint64_t CPUSideNextFenceValue);
		bool ResetAllocatorLists(size_t NumPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type);
		void PushbackAllocatorLists(size_t NumPair, ID3D12CommandAllocator** pAllocatorArray, ID3D12GraphicsCommandList** pListArray, D3D12_COMMAND_LIST_TYPE type);

		bool IsCompletedTask(uint64_t fenceValue = 0);

		HRESULT CloseLists();
		uint64_t ExecuteCommandQueue();
		HRESULT Reset();

	private:
		ID3D12CommandQueue* m_pCommandQueue;
		CustomFence         m_CustomFence;
		std::vector<ID3D12CommandAllocator*>    m_CommandAllocators;
		std::vector<ID3D12GraphicsCommandList*> m_CommandLists;

		D3D12_COMMAND_LIST_TYPE m_Type;
	};
	
	class TaskFiberManager
	{
	public:
		explicit TaskFiberManager(ID3D12Device* pDevice);
		~TaskFiberManager();

		TaskFiber* RequestWorkSubmission(size_t numPairALs, D3D12_COMMAND_LIST_TYPE type, uint64_t completedFenceValue); // pseudo

		// �� �Լ��� ���⿡ �־�ߵǴ°��� �� ��.
		// ���߿� �̵����ɼ� ����.
		// -> GPU�� �����ϰ�, GPUWorkManager�� ȸ��.
		void Release(TaskFiber* pWorkSubmission);
		
	private:
		ID3D12Device* m_pDevice; // = device::g_pDevice; -> Will be Custom::D3D12Adapter::Custom::D3D12Device::m_pDevice

		// COMMAND_LIST_TYPE [DIRECT, COMPUTE, COPY]
		std::vector<TaskFiber> m_ProcessingTask[3]; 

		// CommandAllocator, CommandList
		CustomCommandQALPool m_QALPool[3];
		// ���� �ִ� Fence�� ���� ���� ������� Fence�� �����ؾ���.-> ����
		// -> ������ CustomFence�� ������ ������ ����������?
		// CustomFencePool      m_CustomFencePool; 

		std::mutex m_Mutex;
	};
	
	// ���� ħ�뿡�� 3���� ���ÿ� �Ѵٴ°� ��������...
	// KD Tree, MemoryPool, ���� MultiThreading�̾���
	

	/*
	D3D12_COMMAND_QUEUE, D3D12_Command_Allocator, D3D12_Command_List �з�
	D3D12_COMMAND_LIST_TYPE_DIRECT	= 0,
    D3D12_COMMAND_LIST_TYPE_COMPUTE	= 2,
    D3D12_COMMAND_LIST_TYPE_COPY	= 3,
	*/


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

	// UE4���� �����ؾ��� ��.

	// 1. DescriptorHeap�̳�, ResourceAllocator ����.

	// 2. WorkerThreadContext(���� ���� ����) ��� ��������
	// 2-1. CommandQueue�� CommandList�� ��Ƽ�������� ��� �Ұ���
	// 2-2. WorkerThreadContext(���� ���� ����)�� ��� Thread����, �����ϴ��� -> �� ���� ��� ThreadPool���.


	// ����
	// ���࿡ MemoryPool�̳�, ThreadPool����
	// �ٸ� Allocator(����, �迭Allocator�� ����ٸ�)
	// Return�� RequestAllocate(size_t memorySize, void* pMemoryPool)�� �ϰų�
	// �ƴϸ� ���ø��Լ��� ���� MemoryPool�̳�, ThreadPool���� �޾ƿ� �� �ְ��ϸ� ��������.

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 1���� Adapter�� D3D11, D3D11on12, D3D12 ��� ������ ���� �� �ִٴ°� �������� �˰� �־�����,
	// ���������δ� �ľ��� ������...
	// -> Adapter Ŭ������ �ƿ� ����, ���� 11, 11on12, 12�� ����
	// �ʿ��ѰŸ� ��� �� �� �� �־���.
	// �� ��쿡�� 11on12, 12�� ��� ���°� ���� �̻���.
}
