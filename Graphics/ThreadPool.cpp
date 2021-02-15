#include "stdafx.h"
#include "ThreadPool.h"

namespace custom
{
	ThreadPool* ThreadPool::sm_pThreadPool = nullptr;
	size_t      ThreadPool::sm_LogicalProcessors = 0;

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

		sm_LogicalProcessors = ::GetNumLogicalProcessors() - 1;

		m_hWakeUpEvent     = CreateEvent(nullptr, true, false, nullptr); // ManualInit is true.
		m_hWorkFinishEvent = CreateEvent(nullptr, false, false, nullptr);

		InitializeCriticalSection(&m_OrderCS);
		InitializeCriticalSection(&m_QueueCS);
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
				HANDLE(reinterpret_cast<HANDLE>(
					_beginthreadex(
						nullptr, 0,
						ThreadPool::sm_WorkerProcess,
						reinterpret_cast<LPVOID>(m_ThreadWorkFinishEvents[i]),
						0, nullptr
					)))
			);
		}

		WaitFinishedAllThreads();

		return true;
	}

	void ThreadPool::AddThreads(uint32_t NumAddThreads)
	{
		// Not yet.
		ASSERT(false);
	}
	void ThreadPool::Resize(uint32_t NumNewThreads)
	{
		// Not yet.
		ASSERT(false);
	}
	void ThreadPool::ThreadsShutdown()
	{	
		m_bThreadShutdown = true;
		SetEvent(m_hWakeUpEvent);

		WaitFinishedAllThreads();
		ASSERT(WorkQueueEmpty());
		for (auto& e : m_ThreadWorkFinishEvents)
		{
			CloseHandle(e);
		}
		for (auto& e : m_Threads)
		{
			CloseHandle(e);
		}

		m_ThreadWorkFinishEvents.clear();
		m_Threads.clear();
		CloseHandle(m_hWakeUpEvent);
		CloseHandle(m_hWorkFinishEvent);
		DeleteCriticalSection(&m_OrderCS);
		DeleteCriticalSection(&m_QueueCS);
	}

	STATIC void ThreadPool::Enqueue
	(
		std::function<void(void*)> pWork,
		void* pParameter
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

		EnterCriticalSection(&pThreadPool->m_QueueCS);

		pThreadPool->m_WorkQueue.push_back({ pWork, pParameter });
		SetEvent(pThreadPool->m_hWakeUpEvent);

		LeaveCriticalSection(&pThreadPool->m_QueueCS);
	}

	STATIC void ThreadPool::Enqueue
	(
		std::function<void(void*)>&& pWork,
		void*&& pParameter
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

		EnterCriticalSection(&pThreadPool->m_QueueCS);

		pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, pParameter }));
		SetEvent(pThreadPool->m_hWakeUpEvent);

		LeaveCriticalSection(&pThreadPool->m_QueueCS);
	}

	STATIC void ThreadPool::MultipleEnqueue
	(
		std::function<void(void*)> pWork[],
		size_t ElementSize,
		void* pParameter[]
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

		EnterCriticalSection(&pThreadPool->m_QueueCS);

		if (pParameter)
		{
			for (size_t i = 0; i < ElementSize; ++i)
			{
				pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ *(pWork + i), *(pParameter + i) }));
			}
		}
		else
		{
			for (size_t i = 0; i < ElementSize; ++i)
			{
				pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ *(pWork + i), nullptr }));
			}
		}
		SetEvent(pThreadPool->m_hWakeUpEvent);

		LeaveCriticalSection(&pThreadPool->m_QueueCS);
	}

	STATIC void ThreadPool::WaitAllFinished(HANDLE hFinishEvent /*= nullptr*/)
	{
		std::vector<HANDLE>& FinishEvents = sm_pThreadPool->m_ThreadWorkFinishEvents;
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

		if (!FinishEvents.size())
		{
			return;
		}

		struct FunctionWrapper
		{
			static unsigned int WINAPI ReturnEvent(LPVOID lpParameter)
			{
				std::vector<HANDLE>* pEvents = reinterpret_cast<std::vector<HANDLE>*>(lpParameter);
				HANDLE               FinishHandle = pEvents + 1;

				while (1)
				{
					sm_pThreadPool->WaitFinishedAllThreads();

					if (sm_pThreadPool->WorkQueueEmpty())
					{
						break;
					}
				}

				SetEvent(FinishHandle);
				return 0;
			};

			struct Handles
			{
				std::vector<HANDLE>* pHandles;
				HANDLE pHandle;
			};
		};

		static FunctionWrapper::Handles Param;

		if (hFinishEvent)
		{
			Param.pHandles = &FinishEvents;
			Param.pHandle = hFinishEvent;

			_beginthreadex(nullptr, 0, FunctionWrapper::ReturnEvent, reinterpret_cast<LPVOID>(&Param), 0, nullptr);
			return;
		}
		else
		{
			while (1)
			{
				sm_pThreadPool->WaitFinishedAllThreads();

				while (1)
				{
					if (sm_pThreadPool->WorkQueueEmpty())
					{
						return;
					}
				}
			}
		}
	}

	STATIC unsigned int WINAPI ThreadPool::sm_WorkerProcess(LPVOID pPTR)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;
		HANDLE              PrivateWorkFinishEvent = reinterpret_cast<HANDLE>(pPTR);

		SetEvent(PrivateWorkFinishEvent);

		std::function<void(void*)> Function;
		void* Parameter;

		while (1)
		{
			EnterCriticalSection(&pThreadPool->m_OrderCS);
			WaitForSingleObject(sm_pThreadPool->m_hWakeUpEvent, INFINITE);

			if (!sm_pThreadPool->WorkQueueEmpty())
			{
				{
					EnterCriticalSection(&pThreadPool->m_QueueCS);
					++sm_pThreadPool->m_NumWorkingThreads;

					std::pair<std::function<void(void*)>, void*>& front = pThreadPool->m_WorkQueue.front();
					Function = front.first;
					Parameter = front.second;

					pThreadPool->m_WorkQueue.pop_front();	
					LeaveCriticalSection(&pThreadPool->m_QueueCS);
				}

				LeaveCriticalSection(&pThreadPool->m_OrderCS);
				Function(Parameter);

				SetEvent(PrivateWorkFinishEvent);
			}
			else
			{
				if (!pThreadPool->m_bThreadShutdown)
				{
					ResetEvent(sm_pThreadPool->m_hWakeUpEvent);
					LeaveCriticalSection(&pThreadPool->m_OrderCS);
	            }
				else
				{
					LeaveCriticalSection(&pThreadPool->m_OrderCS);
					SetEvent(PrivateWorkFinishEvent);
					return 0;
				}
			}
		}
	}
}