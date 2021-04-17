#pragma once

// 1. 이제 Fence가 단지 GPU의 CommandQueue의 작업이 끝났는지를 받는 동기화 오브젝트인 것 뿐만 아니라,
//    

namespace custom
{
	// class CustomFencePool;

	class CustomFence
	{
	public:
		explicit CustomFence(D3D12_COMMAND_LIST_TYPE type);
		~CustomFence();
		void Destroy();

		ID3D12Device*       GetShouldReleaseDevice() const { ID3D12Device* pDevice = nullptr; ASSERT_HR(m_prFence->GetDevice(IID_PPV_ARGS(&pDevice))); return pDevice; }
		ID3D12Fence*        GetFence() const           { ASSERT(m_prFence); return m_prFence; }
		ID3D12CommandQueue* GetCommandQueue() const     { ASSERT(m_pCommandQueue); return m_pCommandQueue; }
		HANDLE              GetCompletionEvent() const  { ASSERT(m_hCompleteEvent); return m_hCompleteEvent; }

		uint64_t GetCPUSideNextFenceValue(D3D12_COMMAND_LIST_TYPE expectedtype);
		uint64_t GetGPUCompletedValue(D3D12_COMMAND_LIST_TYPE expectedtype);
		uint64_t GetLastExecuteFenceValue() const { return m_LastExecuteFenceValue; }

		void SetCommandQueue(ID3D12CommandQueue* pCommandQueue);
		void SetLastExecuteFenceValue(uint64_t fenceValue);
		void SignalCPUFenceValue(uint64_t signalValue);
		void SignalCPUGPUFenceValue(uint64_t signalValue);
		uint64_t IncreCPUGPUFenceValue();

		bool IsFenceComplete(uint64_t fenceValue = 0ull);
		HRESULT GPUSideWait(ID3D12Fence* pFence, uint64_t fenceValue);
		HRESULT GPUSideWait(CustomFence& customFence);
		_Check_return_ DWORD CPUSideWait(uint64_t fenceValue = 0,  bool WaitForCompletion = false);
		_Check_return_ DWORD CPUSideWait(CustomFence& customFence, bool WaitForCompletion = false);

	public:
		static void     SetFence(ID3D12Fence* pFence, D3D12_COMMAND_LIST_TYPE type);
		static void     UpdateCPUFenceValue(uint64_t fenceValue, D3D12_COMMAND_LIST_TYPE type);
		static uint64_t IncrementCPUFenceValue(D3D12_COMMAND_LIST_TYPE type);
		static void     DestoryAllFences();

	private:
		// 3D, Compute, Copy
		static ID3D12Fence* sm_pFence[3]; // Thread-Safe.
		static volatile uint64_t sm_CPUSideNextFenceValue[3];
		static volatile uint64_t sm_LastCompletedGPUFenceValue[3];

	private:
		ID3D12Fence*&       m_prFence;
		ID3D12CommandQueue* m_pCommandQueue;
		uint64_t            m_LastExecuteFenceValue;
		volatile uint64_t&  m_rCPUSideNextFenceValue;
		volatile uint64_t&  m_rLastCompletedGPUFenceValue;

		D3D12_COMMAND_LIST_TYPE m_Type;
		HANDLE                  m_hCompleteEvent;
	};
}