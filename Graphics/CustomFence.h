#pragma once

namespace custom
{
	// class CustomFencePool;

	class CustomFence
	{
	public:
		explicit CustomFence(D3D12_COMMAND_LIST_TYPE type);
		~CustomFence();
		void Destroy();

		void Reset(ID3D12Fence* pFence, D3D12_COMMAND_LIST_TYPE type, uint64_t CPUSideNextFenceValue);
		void Reset(CustomFence& pCustomFence, D3D12_COMMAND_LIST_TYPE type, uint64_t CPUSideNextFenceValue);

		ID3D12Device*       GetDevice()           { ID3D12Device* pDevice = nullptr; ASSERT_HR(m_pFence->GetDevice(IID_PPV_ARGS(&pDevice))); return pDevice; }
		ID3D12Fence*        GetFence()            { ASSERT(m_pFence); return m_pFence; }
		ID3D12CommandQueue* GetCommandQueue()     { ASSERT(m_pCommandQueue); return m_pCommandQueue; }
		HANDLE              GetCompletionEvent()  { ASSERT(m_hCompleteEvent); return m_hCompleteEvent; }

		uint64_t GetCPUSideNextFenceValue(D3D12_COMMAND_LIST_TYPE expectedtype);
		uint64_t GetGPUCompletedValue(D3D12_COMMAND_LIST_TYPE expectedtype);

		void SetCommandQueue(ID3D12CommandQueue* pCommandQueue);
		void SetSetGPUFenceValue(uint64_t ToSignalValue);

		uint64_t IncreCPUGPUFenceValue();

		bool IsFenceComplete(uint64_t fenceValue = 0);
		HRESULT GPUSideWait(ID3D12Fence* pFence, uint64_t fenceValue);
		HRESULT GPUSideWait(CustomFence& customFence);
		_Check_return_ DWORD CPUSideWait(uint64_t fenceValue = 0, bool WaitForCompletion = false);
		_Check_return_ DWORD CPUSideWait(CustomFence& customFence, bool WaitForCompletion = false);

	private:
		ID3D12Fence*        m_pFence;
		ID3D12CommandQueue* m_pCommandQueue;
		uint64_t            m_CPUSideNextFenceValue;
		uint64_t            m_LastCompletedGPUFenceValue;

		D3D12_COMMAND_LIST_TYPE m_Type;
		HANDLE                  m_hCompleteEvent;
	};
}