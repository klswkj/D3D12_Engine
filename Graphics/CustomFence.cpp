#include "stdafx.h"
#include "CustomFence.h"

#define CHECK_TYPE(Type, FenceValue) (FenceValue >> 56) == (uint64_t)(Type)
#define CHECK_VALID_FENCE_VALUE(Type, FenceValue) (((uint64_t)Type << 56) <= FenceValue) && \
                                                  (FenceValue <= ((uint64_t)(Type + 1ull) << 56) - 1)
#define CHECK_VALID_TYPE(Type) Type == D3D12_COMMAND_LIST_TYPE_DIRECT || \
                               Type == D3D12_COMMAND_LIST_TYPE_COPY   || \
                               Type == D3D12_COMMAND_LIST_TYPE_COMPUTE

namespace custom
{
	// pFence를 Set하면 GetCompletedValue로 GPUFenceValue는 알 수 있음.
	// GPUSideFenceValue는 (uint64_t)Type << 56보다 커야되고,
	// CPUSideNextFenceValue는 적어도 (uint64_t)Type << 56 | 1 보다 커야됨.

	CustomFence::CustomFence(D3D12_COMMAND_LIST_TYPE type)
		:
		m_pFence(nullptr),
		m_pCommandQueue(nullptr),
		m_CPUSideNextFenceValue(0),
		m_LastCompletedGPUFenceValue(0),
		m_Type(type)
	{
		ASSERT(CHECK_VALID_TYPE(type));
		m_hCompleteEvent = CreateEvent(nullptr, false, false, nullptr);
	}

	void CustomFence::Reset(ID3D12Fence* pFence, D3D12_COMMAND_LIST_TYPE type, uint64_t CPUSideNextFenceValue)
	{
		ASSERT(pFence);
		ASSERT(CHECK_VALID_TYPE(type));
		uint64_t LastCompletedFenceValue = pFence->GetCompletedValue();
		ASSERT(CHECK_VALID_FENCE_VALUE(type, LastCompletedFenceValue));
		ASSERT(CHECK_VALID_FENCE_VALUE(type, CPUSideNextFenceValue));
		ASSERT(LastCompletedFenceValue < CPUSideNextFenceValue);

		m_pFence                     = pFence;
		m_Type                       = type;
		m_CPUSideNextFenceValue      = CPUSideNextFenceValue;
		m_LastCompletedGPUFenceValue = LastCompletedFenceValue;
		ResetEvent(&m_hCompleteEvent);
	}
	void CustomFence::Reset(CustomFence& pCustomFence, D3D12_COMMAND_LIST_TYPE type, uint64_t CPUSideNextFenceValue)
	{
		ID3D12Fence* pFence = pCustomFence.GetFence();
		Reset(pFence, type, CPUSideNextFenceValue);
	}

	CustomFence::~CustomFence()
	{
		Destroy();
	}

	void CustomFence::Destroy()
	{
		if (m_hCompleteEvent)
		{
			ASSERT(CloseHandle(m_hCompleteEvent));
		}
		m_hCompleteEvent = nullptr;
		// SafeRelease(m_pFence); -> TODO 1 : Decrement RefCount
		// m_pFence.RefDecr;
	}
	uint64_t CustomFence::GetCPUSideNextFenceValue(D3D12_COMMAND_LIST_TYPE expectedtype)
	{
		ASSERT(m_pFence);
		ASSERT(expectedtype == m_Type);
		ASSERT(CHECK_TYPE(expectedtype, m_CPUSideNextFenceValue));

		return m_CPUSideNextFenceValue;
	}

	uint64_t CustomFence::GetGPUCompletedValue(D3D12_COMMAND_LIST_TYPE expectedtype)
	{
		ASSERT(m_pFence);
		ASSERT(expectedtype == m_Type);

		uint64_t CurrentGPUSideFenceValue = m_pFence->GetCompletedValue();

		ASSERT(CHECK_TYPE(expectedtype, CurrentGPUSideFenceValue));

		m_LastCompletedGPUFenceValue = 
			(CurrentGPUSideFenceValue < m_LastCompletedGPUFenceValue) ?
			m_LastCompletedGPUFenceValue :
			CurrentGPUSideFenceValue;

		return m_LastCompletedGPUFenceValue;
	}

	void CustomFence::SetSetGPUFenceValue(uint64_t ToSignalValue)
	{
		ASSERT(CHECK_VALID_FENCE_VALUE(m_Type, ToSignalValue));
		m_pFence->Signal(ToSignalValue);
	}

	void CustomFence::SetCommandQueue(ID3D12CommandQueue* pCommandQueue)
	{
		ASSERT(pCommandQueue);
		D3D12_COMMAND_LIST_TYPE NewType = pCommandQueue->GetDesc().Type;

		ASSERT(CHECK_VALID_TYPE(NewType));
		ASSERT(m_Type == NewType);

		m_pCommandQueue = pCommandQueue;
	}

	uint64_t CustomFence::IncreCPUGPUFenceValue()
	{
		ASSERT(m_pCommandQueue);
		m_pCommandQueue->Signal(m_pFence, m_CPUSideNextFenceValue);
		return m_CPUSideNextFenceValue++;
	}

	bool CustomFence::IsFenceComplete(uint64_t fenceValue)
	{
		if (fenceValue == 0)
		{
			m_LastCompletedGPUFenceValue =
				(m_pFence->GetCompletedValue() < m_LastCompletedGPUFenceValue) ?
				m_LastCompletedGPUFenceValue :
				m_pFence->GetCompletedValue();

			return m_pFence->GetCompletedValue() == m_CPUSideNextFenceValue - 1;
		}
		else
		{
			ASSERT(CHECK_VALID_FENCE_VALUE(m_Type, fenceValue));

			if (m_LastCompletedGPUFenceValue < fenceValue)
			{
				m_LastCompletedGPUFenceValue =
					(m_pFence->GetCompletedValue() < m_LastCompletedGPUFenceValue) ?
					m_LastCompletedGPUFenceValue :
					m_pFence->GetCompletedValue();
			}

			return fenceValue <= m_LastCompletedGPUFenceValue;
		}
	}
	HRESULT CustomFence::GPUSideWait(ID3D12Fence* pFence, uint64_t fenceValue)
	{
		ASSERT(pFence && fenceValue);
		ASSERT(m_pCommandQueue);
		ASSERT(CHECK_VALID_FENCE_VALUE(m_Type, fenceValue));

		return m_pCommandQueue->Wait(pFence, fenceValue);
	}
	HRESULT CustomFence::GPUSideWait(CustomFence& customFence)
	{
		uint64_t FenceValue = customFence.GetCPUSideNextFenceValue();
		ID3D12Fence* pFence = customFence.GetFence();

		ASSERT(pFence && FenceValue);

		return GPUSideWait(pFence, FenceValue);
	}

	DWORD CustomFence::CPUSideWait(uint64_t fenceValue, bool WaitForCompletion)
	{
		if (fenceValue == 0)
		{
			ASSERT(m_pFence->GetCompletedValue() <= m_CPUSideNextFenceValue);

			ASSERT_HR(m_pFence->SetEventOnCompletion(IncreCPUGPUFenceValue(), m_hCompleteEvent));
			DWORD Result = ::WaitForSingleObject(m_hCompleteEvent, WaitForCompletion ? INFINITE : 0);

			return Result;
		}

		ASSERT(CHECK_VALID_FENCE_VALUE(m_Type, fenceValue));

		if (IsFenceComplete(fenceValue))
		{
			return WAIT_OBJECT_0;
		}
		else
		{
			ASSERT_HR(m_pFence->SetEventOnCompletion(fenceValue, m_hCompleteEvent));
			DWORD Result = ::WaitForSingleObject(m_hCompleteEvent, WaitForCompletion ? INFINITE : 0);

			return Result;
		}

		return S_FALSE;
	}

	DWORD CustomFence::CPUSideWait(CustomFence& customFence, bool WaitForCompletion)
	{
		uint64_t FenceValue = customFence.GetCPUSideNextFenceValue();

		ASSERT(FenceValue);
		ASSERT_HR(m_pFence->SetEventOnCompletion(--FenceValue, m_hCompleteEvent));
		DWORD Result = ::WaitForSingleObject(m_hCompleteEvent, WaitForCompletion ? INFINITE : 0);

		return Result;
	}
}

/*
namespace custom
{
	CustomFencePool::CustomFencePool()
		: m_pDevice(nullptr)
	{
	}
	CustomFencePool::CustomFencePool(ID3D12Device* pDevice)
	{
		ASSERT(pDevice);
		m_pDevice = pDevice;
	}
	CustomFencePool::~CustomFencePool()
	{
		Shutdown();
	}

	void CustomFencePool::Shutdown()
	{
		for (auto e : m_AllFences)
		{
			e->Destroy();
		}
	}

	CustomFence* CustomFencePool::RequestCustomFence()
	{
		CustomFence* pCustomFence = nullptr;

		std::lock_guard<std::mutex> lockGuard(m_Mutex);

		if (!m_AvailableFences.empty())
		{
			pCustomFence = m_AvailableFences.front();

			m_AvailableFences.pop_front();
			// --m_NumAvailableCommandQueues;
		}
		else
		{
			pCustomFence = new CustomFence(m_pDevice, this);

#ifdef _DEBUG
			pCustomFence->GetFence()->SetName(StringToWString("CustomFencePOOL #" + std::to_string(m_AllFences.size())).c_str());
#endif

			m_AllFences.push_back(pCustomFence);
		}

		ASSERT(pCustomFence);
		return pCustomFence;
	}
}
*/