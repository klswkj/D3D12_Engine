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

		// 이 함수가 여기에 있어야되는건지 잘 모름.
		// 나중에 이동가능성 농후.
		// -> GPU에 전송하고, GPUWorkManager로 회수.
		void Release(TaskFiber* pWorkSubmission);
		
	private:
		ID3D12Device* m_pDevice; // = device::g_pDevice; -> Will be Custom::D3D12Adapter::Custom::D3D12Device::m_pDevice

		// COMMAND_LIST_TYPE [DIRECT, COMPUTE, COPY]
		std::vector<TaskFiber> m_ProcessingTask[3]; 

		// CommandAllocator, CommandList
		CustomCommandQALPool m_QALPool[3];
		// 여기 있는 Fence는 밖의 같은 엔진들과 Fence를 같이해야함.-> 삭제
		// -> 하지만 CustomFence의 개념은 건져서 쓸말할지도?
		// CustomFencePool      m_CustomFencePool; 

		std::mutex m_Mutex;
	};
	
	// 어제 침대에서 3개를 동시에 한다는게 뭐였더라...
	// KD Tree, MemoryPool, 현재 MultiThreading이었나
	

	/*
	D3D12_COMMAND_QUEUE, D3D12_Command_Allocator, D3D12_Command_List 분류
	D3D12_COMMAND_LIST_TYPE_DIRECT	= 0,
    D3D12_COMMAND_LIST_TYPE_COMPUTE	= 2,
    D3D12_COMMAND_LIST_TYPE_COPY	= 3,
	*/


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

	// UE4에서 돚거해야할 거.

	// 1. DescriptorHeap이나, ResourceAllocator 돚거.

	// 2. WorkerThreadContext(내가 붙인 가명) 어떻게 구성할지
	// 2-1. CommandQueue와 CommandList를 멀티스레딩을 어떻게 할건지
	// 2-2. WorkerThreadContext(내가 붙인 가명)을 어떻게 Thread생성, 관리하는지 -> 나 같은 경우 ThreadPool사용.


	// 배운거
	// 만약에 MemoryPool이나, ThreadPool에서
	// 다른 Allocator(가령, 배열Allocator를 만든다면)
	// Return형 RequestAllocate(size_t memorySize, void* pMemoryPool)로 하거나
	// 아니면 템플릿함수를 만들어서 MemoryPool이나, ThreadPool에서 받아올 수 있게하면 괜찮더라.

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 1개의 Adapter로 D3D11, D3D11on12, D3D12 등등 여러개 만들 수 있다는건 경험으로 알고 있었지만,
	// 지식적으로는 파악을 못했음...
	// -> Adapter 클래스로 아예 만들어서, 여하 11, 11on12, 12를 만들어서
	// 필요한거를 골라서 쓸 수 도 있었음.
	// 내 경우에는 11on12, 12를 골라서 쓰는게 가장 이상적.
}
