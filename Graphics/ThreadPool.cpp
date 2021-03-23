#include "stdafx.h"
#include "ThreadPool.h"

#pragma warning(push)
#pragma warning (disable : 28251)

namespace custom
{
	ThreadPool* ThreadPool::sm_pThreadPool = nullptr;
	uint32_t    ThreadPool::sm_LogicalProcessors = 0u;

	ThreadPool::ThreadPool()
		:
		m_OrderCS({}),
		m_QueueCS({}),
		m_hWakeUpEvent(nullptr),
		m_hWorkFinishEvent(nullptr),
		m_bThreadShutdown(false),
		m_NumWorkingThreads(0)
	{
		ASSERT(sm_pThreadPool == nullptr);
		sm_pThreadPool = this;

		sm_LogicalProcessors = GetNumLogicalProcessors() - 1u;
		
		m_hWakeUpEvent     = CreateEvent(nullptr, true, true, nullptr); // ManualInit, InitState is true.
		m_hWorkFinishEvent = CreateEvent(nullptr, true, true, nullptr);

		ASSERT(m_hWakeUpEvent && m_hWorkFinishEvent && sm_LogicalProcessors < 0x40ull);

		::InitializeCriticalSection(&m_OrderCS);
		::InitializeCriticalSection(&m_QueueCS);
	}
	ThreadPool::~ThreadPool()
	{
		ThreadsShutdown();
	}

	bool ThreadPool::Create(uint32_t NumThreads/*= 0*/)
	{
		size_t NumMakeThreads = 0;

		if (NumThreads == 0)
		{
			NumMakeThreads = sm_LogicalProcessors;
		}
		else
		{
			ASSERT(NumThreads < uint32_t(sm_LogicalProcessors * 2));
			NumMakeThreads = NumThreads;
		}

		m_ThreadWorkFinishEvents.resize(NumMakeThreads, nullptr);

		for (auto& e : m_ThreadWorkFinishEvents)
		{
			e = CreateEvent(nullptr, false, false, nullptr);
		}

		m_Threads.reserve(NumMakeThreads);

		for (size_t i = 0; i < NumMakeThreads; ++i)
		{
			m_Threads.emplace_back
			(
				HANDLE(reinterpret_cast<HANDLE>(::_beginthreadex(
						nullptr, 0,
						ThreadPool::sm_WorkerProcess,
						reinterpret_cast<LPVOID>(m_ThreadWorkFinishEvents[i]),
						0, nullptr)))
			);
		}

		WaitFinishedAllThreads();

		return true;
	}

	void ThreadPool::AddThreads(uint32_t NumAddThreads)
	{
		// Not implementation yet.
		ASSERT(false);
	}
	void ThreadPool::Resize(uint32_t NumNewThreads)
	{
		// Not implementation yet.
		ASSERT(false);
	}
	void ThreadPool::ThreadsShutdown()
	{
		WaitFinishedAllThreads();

		m_bThreadShutdown = true;
		if (!::SetEvent(m_hWakeUpEvent))
		{
			HRESULT hardwareResult = HRESULT_FROM_WIN32(GetLastError());
			ASSERT_HR(hardwareResult);
		}
		::WaitForSingleObject(m_hWorkFinishEvent, INFINITE);
		::Sleep(100);
		WaitFinishedAllThreads();

		for (auto& e : m_ThreadWorkFinishEvents)
		{
			::CloseHandle(e);
			e = INVALID_HANDLE_VALUE;
		}
		for (auto& e : m_Threads)
		{
			::CloseHandle(e);
			e = INVALID_HANDLE_VALUE;
		}

		m_ThreadWorkFinishEvents.clear();
		m_Threads.clear();
		::CloseHandle(m_hWakeUpEvent);
		m_hWakeUpEvent = INVALID_HANDLE_VALUE;
		::CloseHandle(m_hWorkFinishEvent);
		m_hWorkFinishEvent = INVALID_HANDLE_VALUE;
		::DeleteCriticalSection(&m_OrderCS);
		m_OrderCS.DebugInfo = 0;
		::DeleteCriticalSection(&m_QueueCS);
		m_QueueCS.DebugInfo = 0;
	}

	STATIC void ThreadPool::Enqueue
	(
		std::function<void(void*)> pWork,
		void* pParameter
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

		::EnterCriticalSection(&pThreadPool->m_QueueCS);

		pThreadPool->m_WorkQueue.push_back({ pWork, pParameter });
		::SetEvent(pThreadPool->m_hWakeUpEvent);

		::LeaveCriticalSection(&pThreadPool->m_QueueCS);
	}

	STATIC void ThreadPool::Enqueue
	(
		std::function<void(void*)>&& pWork,
		void*&& pParameter
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

		::EnterCriticalSection(&pThreadPool->m_QueueCS);

		pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, pParameter }));
		SetEvent(pThreadPool->m_hWakeUpEvent);

		::LeaveCriticalSection(&pThreadPool->m_QueueCS);
	}

	STATIC void ThreadPool::EnqueueVariadic
	(
		_In_ std::function<void(void*)>&& pWork,
		int NumParam, ...
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

		va_list VaList;
		va_start(VaList, NumParam);

		::EnterCriticalSection(&pThreadPool->m_QueueCS);

		pThreadPool->m_WorkQueue.emplace_back(::std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, VaList }));
		::SetEvent(pThreadPool->m_hWakeUpEvent);

		::LeaveCriticalSection(&pThreadPool->m_QueueCS);
	}

	STATIC void ThreadPool::MultipleEnqueue
	(
		std::function<void(void*)> pWorks[],
		size_t numElements,
		void* pParameters[],
		size_t ElementSize
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

		::EnterCriticalSection(&pThreadPool->m_QueueCS);

		if (pParameters)
		{
			if (ElementSize)
			{
				for (size_t i = 0; i < numElements; ++i)
				{
					pThreadPool->m_WorkQueue.emplace_back(::std::forward<std::pair<std::function<void(void*)>, void*>>({ *(pWorks + i), reinterpret_cast<void*>(reinterpret_cast<size_t>(pParameters) + i * ElementSize) }));
				}
			}
			else
			{
				for (size_t i = 0; i < numElements; ++i)
				{
					pThreadPool->m_WorkQueue.emplace_back(::std::forward<std::pair<std::function<void(void*)>, void*>>({ *(pWorks + i), *(pParameters + i) }));
				}
			}
		}
		else
		{
			for (size_t i = 0; i < numElements; ++i)
			{
				pThreadPool->m_WorkQueue.emplace_back(::std::forward<std::pair<std::function<void(void*)>, void*>>({ *(pWorks + i), nullptr }));
			}
		}
		::SetEvent(pThreadPool->m_hWakeUpEvent);

		::LeaveCriticalSection(&pThreadPool->m_QueueCS);
	}

	STATIC void ThreadPool::MultipleEnqueue
	(
		_In_ std::function<void(void*)> pWork,
		_In_ size_t numElements,
		_In_opt_ void* pParameters[],
		_In_opt_ size_t ElementSize
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;
		::EnterCriticalSection(&pThreadPool->m_QueueCS);

		if (pParameters)
		{
			if (ElementSize)
			{
				for (size_t i = 0ul; i < numElements; ++i)
				{
					pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, reinterpret_cast<void*>(reinterpret_cast<size_t>(pParameters) + i * ElementSize) }));
				}
			}
			else
			{
				for (size_t i = 0ul; i < numElements; ++i)
				{
					pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, *(pParameters + i) }));
				}
			}
		}
		else
		{
			for (size_t i = 0ul; i < numElements; ++i)
			{
				pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, nullptr }));
			}
		}

		::SetEvent(pThreadPool->m_hWakeUpEvent);
		::LeaveCriticalSection(&pThreadPool->m_QueueCS);
	}

	STATIC void ThreadPool::WaitAllFinished(HANDLE hFinishEvent /*= nullptr*/)
	{
		std::vector<HANDLE>& FinishEvents = sm_pThreadPool->m_ThreadWorkFinishEvents;
		custom::ThreadPool*  pThreadPool  = sm_pThreadPool;

		struct FunctionWrapper
		{
			static unsigned int WINAPI ReturnEvent(LPVOID lpParameter)
			{
				std::vector<HANDLE>* pEvents = reinterpret_cast<std::vector<HANDLE>*>(lpParameter);
				HANDLE               FinishHandle = pEvents + 1ul;

				while (1)
				{
					if (sm_pThreadPool->WorkQueueEmpty())
					{
						break;
					}
				}

				sm_pThreadPool->WaitFinishedAllThreads();

				::SetEvent(FinishHandle);
				return 0;
			};

			struct Handles
			{
				std::vector<HANDLE>* pHandles;
				HANDLE pHandle;
			};
		};

		static FunctionWrapper::Handles Param = {};

		if (hFinishEvent)
		{
			Param.pHandles = &FinishEvents;
			Param.pHandle = hFinishEvent;

			::_beginthreadex(nullptr, 0, FunctionWrapper::ReturnEvent, reinterpret_cast<LPVOID>(&Param), 0ul, nullptr);
			return;
		}
		else
		{
			while (1)
			{
				if (sm_pThreadPool->WorkQueueEmpty())
				{
					break;
				}
			}

			sm_pThreadPool->WaitFinishedAllThreads();
		}

	}

	STATIC unsigned int WINAPI ThreadPool::sm_WorkerProcess(LPVOID pPTR)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;
		HANDLE              PrivateWorkFinishEvent = reinterpret_cast<HANDLE>(pPTR);

		::SetEvent(PrivateWorkFinishEvent);

		std::function<void(void*)> Function;
		void* Parameter;

		while (1)
		{
			::EnterCriticalSection(&pThreadPool->m_OrderCS);
			::WaitForSingleObject(pThreadPool->m_hWakeUpEvent, INFINITE);

			if (!pThreadPool->WorkQueueEmpty())
			{
				::EnterCriticalSection(&pThreadPool->m_QueueCS);
				::ResetEvent(pThreadPool->m_hWorkFinishEvent);
				::InterlockedIncrement(&pThreadPool->m_NumWorkingThreads);
				{
					std::pair<std::function<void(void*)>, void*>& front = pThreadPool->m_WorkQueue.front();
					Function  = front.first;
					Parameter = front.second;
				}
				pThreadPool->m_WorkQueue.pop_front();

				::LeaveCriticalSection(&pThreadPool->m_QueueCS);
				::LeaveCriticalSection(&pThreadPool->m_OrderCS);

				Function(Parameter);

				::InterlockedDecrement(&pThreadPool->m_NumWorkingThreads);
				::SetEvent(PrivateWorkFinishEvent);
			}
			else
			{
				if (!pThreadPool->m_bThreadShutdown)
				{
					::ResetEvent(pThreadPool->m_hWakeUpEvent);
					::SetEvent(pThreadPool->m_hWorkFinishEvent);
					::LeaveCriticalSection(&pThreadPool->m_OrderCS);
	            }
				else
				{
					::LeaveCriticalSection(&pThreadPool->m_OrderCS);
					::SetEvent(PrivateWorkFinishEvent);
					::SetEvent(pThreadPool->m_hWorkFinishEvent);
					return 0;
				}
			}
		}
	}
}

#pragma warning(pop)