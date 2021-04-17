#include "stdafx.h"
#include "CustomFence.h"

namespace custom
{
	ID3D12Fence* CustomFence::sm_pFence[3] = { nullptr, nullptr, nullptr };

	volatile uint64_t CustomFence::sm_CPUSideNextFenceValue[3] =
	{
		((uint64_t)D3D12_COMMAND_LIST_TYPE_DIRECT  << 56) | 1,
		((uint64_t)D3D12_COMMAND_LIST_TYPE_COMPUTE << 56) | 1,
		((uint64_t)D3D12_COMMAND_LIST_TYPE_COPY    << 56) | 1
	};
	volatile uint64_t CustomFence::sm_LastCompletedGPUFenceValue[3] =
	{
		((uint64_t)D3D12_COMMAND_LIST_TYPE_DIRECT  << 56),
		((uint64_t)D3D12_COMMAND_LIST_TYPE_COMPUTE << 56),
		((uint64_t)D3D12_COMMAND_LIST_TYPE_COPY    << 56),
	};

	CustomFence::CustomFence(D3D12_COMMAND_LIST_TYPE type)
		:
		m_prFence(sm_pFence[TYPE_TO_INDEX(type)]),
		m_pCommandQueue(nullptr),
		m_LastExecuteFenceValue((uint64_t)type << 56ull),
		m_rCPUSideNextFenceValue(sm_CPUSideNextFenceValue[TYPE_TO_INDEX(type)]),
		m_rLastCompletedGPUFenceValue(sm_LastCompletedGPUFenceValue[TYPE_TO_INDEX(type)]),
		m_Type(type)
	{
		ASSERT(CHECK_VALID_TYPE(type) && m_prFence, "CustomFence::SetFence Must be called.");
		m_hCompleteEvent = CreateEvent(nullptr, false, false, nullptr);
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

	STATIC void CustomFence::SetFence(ID3D12Fence* pFence, D3D12_COMMAND_LIST_TYPE type)
	{
		uint64_t Index = TYPE_TO_INDEX(type);
		uint64_t GPUSideLastCompletedFenceValue = pFence->GetCompletedValue();

		ASSERT
		(
			pFence && 
			CHECK_VALID_TYPE(type) && 
			(sm_pFence[Index] == nullptr) && 
			CHECK_VALID_FENCE_VALUE(type, pFence->GetCompletedValue()) && 
			GPUSideLastCompletedFenceValue < InterlockedGetValue(&sm_CPUSideNextFenceValue[Index])
		);

		sm_pFence[Index] = pFence;
	}

	STATIC uint64_t CustomFence::IncrementCPUFenceValue(D3D12_COMMAND_LIST_TYPE type)
	{
		ASSERT(CHECK_VALID_TYPE(type));
		return InterlockedIncrement64((volatile LONG64*)&sm_CPUSideNextFenceValue[TYPE_TO_INDEX(type)]);
		// return ++sm_CPUSideNextFenceValue[TYPE_TO_INDEX(type)]);
	}

	STATIC void CustomFence::UpdateCPUFenceValue(uint64_t fenceValue, D3D12_COMMAND_LIST_TYPE type)
	{
		// Will be deleted.
		// ASSERT(CHECK_VALID_TYPE(type));
		// ASSERT(CHECK_VALID_FENCE_VALUE(type, fenceValue));
		// ASSERT(sm_CPUSideNextFenceValue[TYPE_TO_INDEX(type)] <= fenceValue);
		// InterlockedExchange(&sm_CPUSideNextFenceValue[TYPE_TO_INDEX(type)], fenceValue);
	}

	STATIC void CustomFence::DestoryAllFences()
	{
		for (size_t i = 0ul; i < _countof(sm_pFence); ++i)
		{
			if (sm_pFence[i])
			{
				sm_pFence[i]->Release();
				sm_pFence[i] = nullptr;
			}
		}
	}

	uint64_t CustomFence::GetCPUSideNextFenceValue(D3D12_COMMAND_LIST_TYPE expectedtype)
	{
		uint64_t CPUSideNextFenceValue = InterlockedGetValue(&m_rCPUSideNextFenceValue);

		ASSERT
		(
			(expectedtype == m_Type) &&
			CHECK_TYPE(expectedtype, CPUSideNextFenceValue)
		);

		return CPUSideNextFenceValue;
	}

	uint64_t CustomFence::GetGPUCompletedValue(D3D12_COMMAND_LIST_TYPE expectedtype)
	{
		ASSERT
		(
			m_prFence &&
			(expectedtype == m_Type)
		);

		uint64_t CurrentGPUSideFenceValue = m_prFence->GetCompletedValue();

		ASSERT(CHECK_TYPE(expectedtype, CurrentGPUSideFenceValue));

		// m_rLastCompletedGPUFenceValue = 
		// 	(CurrentGPUSideFenceValue <= m_rLastCompletedGPUFenceValue) ?
		// 	m_rLastCompletedGPUFenceValue :
		// 	CurrentGPUSideFenceValue;

		// 의심.
		InterlockedExchange
		(
			&m_rLastCompletedGPUFenceValue, 
			(CurrentGPUSideFenceValue <= m_rLastCompletedGPUFenceValue) 
			? m_rLastCompletedGPUFenceValue : CurrentGPUSideFenceValue
		);

		return InterlockedGetValue(&m_rLastCompletedGPUFenceValue);
	}

	void CustomFence::SetCommandQueue(ID3D12CommandQueue* pCommandQueue)
	{
		ASSERT(pCommandQueue);
		D3D12_COMMAND_LIST_TYPE NewType = pCommandQueue->GetDesc().Type;

		ASSERT
		(
			CHECK_VALID_TYPE(NewType) &&
			(m_Type == NewType)
		);

		m_pCommandQueue = pCommandQueue;
	}
	void CustomFence::SetLastExecuteFenceValue(uint64_t fenceValue)
	{
		ASSERT
		(
			(m_LastExecuteFenceValue < fenceValue) &&
			CHECK_VALID_FENCE_VALUE(m_Type, fenceValue)
		);

		m_LastExecuteFenceValue = fenceValue;
	}

	void CustomFence::SignalCPUFenceValue(uint64_t signalValue)
	{
		ASSERT
		(
			CHECK_VALID_FENCE_VALUE(m_Type, signalValue) &&
			m_prFence
		);

		m_prFence->Signal(signalValue);
	}
	void CustomFence::SignalCPUGPUFenceValue(uint64_t signalValue)
	{
		ASSERT
		(
			CHECK_VALID_FENCE_VALUE(m_Type, signalValue) &&
			m_prFence && m_pCommandQueue
		);
		m_pCommandQueue->Signal(m_prFence, signalValue);
	}

	uint64_t CustomFence::IncreCPUGPUFenceValue()
	{
		ASSERT(m_pCommandQueue && m_prFence);
		m_pCommandQueue->Signal(m_prFence, InterlockedGetValue(&m_rCPUSideNextFenceValue));
		return InterlockedIncrement64((volatile LONG64*)&m_rCPUSideNextFenceValue) - 1;
		// return m_rCPUSideNextFenceValue++;
	}

	bool CustomFence::IsFenceComplete(uint64_t fenceValue /*= 0*/)
	{
		if (fenceValue == 0)
		{
			fenceValue = m_LastExecuteFenceValue;
		}

		ASSERT(m_prFence && CHECK_VALID_FENCE_VALUE(m_Type, fenceValue));

		uint64_t LastCompletedGPUFenceValue = InterlockedGetValue(&m_rLastCompletedGPUFenceValue);

		if (LastCompletedGPUFenceValue <= fenceValue)
		{
			//  m_rLastCompletedGPUFenceValue =
			// 	(m_prFence->GetCompletedValue() <= m_rLastCompletedGPUFenceValue) ?
			// 	m_rLastCompletedGPUFenceValue :
			// 	m_prFence->GetCompletedValue();

			uint64_t GPUSideLastCompletedFenceValue = m_prFence->GetCompletedValue();

				InterlockedExchange64
				(
					(volatile LONG64*)m_rLastCompletedGPUFenceValue, 
					(GPUSideLastCompletedFenceValue <= LastCompletedGPUFenceValue)
					? LastCompletedGPUFenceValue : GPUSideLastCompletedFenceValue
				);
		}

		return fenceValue <= InterlockedGetValue(&m_rLastCompletedGPUFenceValue);

	}
	HRESULT CustomFence::GPUSideWait(ID3D12Fence* pFence, uint64_t fenceValue)
	{
		ASSERT
		(
			pFence && fenceValue && m_pCommandQueue &&
			CHECK_VALID_TYPE((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56))
		);

		return m_pCommandQueue->Wait(pFence, fenceValue);
	}
	HRESULT CustomFence::GPUSideWait(CustomFence& customFence)
	{
		uint64_t FenceValue = customFence.GetCPUSideNextFenceValue(m_Type);
		ID3D12Fence* pFence = customFence.GetFence();

		ASSERT(pFence && FenceValue);

		return GPUSideWait(pFence, FenceValue);
	}

	DWORD CustomFence::CPUSideWait(uint64_t fenceValue, bool WaitForCompletion)
	{
		if (fenceValue == 0)
		{
			fenceValue = m_LastExecuteFenceValue;
		}

		ASSERT(m_prFence && CHECK_VALID_FENCE_VALUE(m_Type, fenceValue));

		if (IsFenceComplete(fenceValue))
		{
			return WAIT_OBJECT_0;
		}
		else
		{
			ASSERT_HR(m_prFence->SetEventOnCompletion(fenceValue, m_hCompleteEvent));
			DWORD Result = ::WaitForSingleObject(m_hCompleteEvent, WaitForCompletion ? INFINITE : 0);

			// fenceValue 업데이트
			InterlockedExchange64
			(
				(volatile LONG64*)m_rLastCompletedGPUFenceValue,
				(fenceValue <= m_rLastCompletedGPUFenceValue)
				? m_rLastCompletedGPUFenceValue : fenceValue
			);

			return Result;
		}

		return WAIT_FAILED;
	}

	DWORD CustomFence::CPUSideWait(CustomFence& customFence, bool WaitForCompletion)
	{
		uint64_t FenceValue = customFence.GetLastExecuteFenceValue();
		D3D12_COMMAND_LIST_TYPE Type = customFence.m_Type;

		ASSERT(m_prFence && CHECK_VALID_FENCE_VALUE(Type, FenceValue));

		if (FenceValue < customFence.GetGPUCompletedValue(Type))
		{
			return WAIT_OBJECT_0;
		}

		ASSERT_HR(m_prFence->SetEventOnCompletion(FenceValue, m_hCompleteEvent));
		DWORD Result = ::WaitForSingleObject(m_hCompleteEvent, WaitForCompletion ? INFINITE : 0);

		return Result;
	}
}