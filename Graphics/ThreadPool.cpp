#include "stdafx.h"
#include "ThreadPool.h"

#define STATIC

namespace custom
{
	ThreadPool* ThreadPool::sm_pThreadPool = nullptr;

	ThreadPool::ThreadPool()
		:
#if defined(USE_CV)
#else
		m_hWakeUpEvent(nullptr),
		m_hWorkFinishEvent(nullptr),
#endif
		m_bThreadShutdown(false),
		m_NumPhysicalCores(0),
		m_NumWorkingThreads(0)
	{
		ASSERT(sm_pThreadPool == nullptr);
		sm_pThreadPool = this;
		m_NumPhysicalCores = GetNumberOfCores();
#if defined(USE_CV)
#else
		m_hWakeUpEvent     = CreateEvent(nullptr, true, false, nullptr); // ManualInit is true.
		m_hWorkFinishEvent = CreateEvent(nullptr, false, false, nullptr);
#endif
	}
	ThreadPool::~ThreadPool()
	{
		ThreadsShutdown();
	}

	bool ThreadPool::Create(uint32_t NumThreads)
	{
		size_t NumMakeThreads = 0;

		if (NumThreads == -1)
		{
			NumMakeThreads = m_NumPhysicalCores;
		}
		else
		{
			ASSERT(NumThreads < uint32_t(m_NumPhysicalCores * 2));
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
#if defined(USE_CV)
		std::unique_lock<std::mutex> UniqueLock(m_QueueMutex);
		m_bThreadShutdown = true;
		UniqueLock.unlock();
		m_WakeUpCV.notify_all();

		WaitFinishedAllThreads();
#else		
		m_bThreadShutdown = true;
		SetEvent(m_hWakeUpEvent);
#endif
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
	}

	STATIC void ThreadPool::Enqueue
	(
		std::function<void(void*)> pWork,
		void* pParameter
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;
#if defined(USE_CV)
		std::unique_lock<std::mutex> UniqueLock(pThreadPool->m_QueueMutex);
		pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, pParameter }));
		pThreadPool->m_WakeUpCV.notify_one();
#else
		std::lock_guard<std::mutex> s_lockguardTriplets1(pThreadPool->m_QueueMutex);
		pThreadPool->m_WorkQueue.push_back({ pWork, pParameter });
		// pThreadPool->m_WorkQueue.emplace_back((pWork, pParameter));
		SetEvent(pThreadPool->m_hWakeUpEvent);
#endif
	}

	STATIC void ThreadPool::Enqueue
	(
		std::function<void(void*)>&& pWork,
		void*&& pParameter
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

#if defined(USE_CV)
		std::unique_lock<std::mutex> UniqueLock(pThreadPool->m_QueueMutex);
		pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, pParameter }));
		pThreadPool->m_WakeUpCV.notify_one();
#else
		std::lock_guard<std::mutex> lockguardTwin1(pThreadPool->m_QueueMutex);
		pThreadPool->m_WorkQueue.emplace_back(std::forward<std::pair<std::function<void(void*)>, void*>>({ pWork, pParameter }));
		SetEvent(pThreadPool->m_hWakeUpEvent);
#endif
	}

	STATIC void ThreadPool::MultipleEnqueue
	(
		std::function<void(void*)> pWork[],
		size_t ElementSize,
		void* pParameter[]
	)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;

#if defined(USE_CV)
		std::unique_lock<std::mutex> UniqueLock(pThreadPool->m_QueueMutex);

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

		UniqueLock.unlock();
		pThreadPool->m_WakeUpCV.notify_all();
#else
		pThreadPool->m_QueueMutex.lock();

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
		pThreadPool->m_QueueMutex.unlock();
#endif
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
#if defined(USE_CV)
			std::unique_lock<std::mutex> UniqueLock(pThreadPool->m_QueueMutex);
			pThreadPool->m_WorkFinishedCV.wait(UniqueLock, [pThreadPool]() {return pThreadPool->WorkQueueEmpty() && !pThreadPool->m_NumWorkingThreads; });
#else
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
#endif
		}
	}

	STATIC unsigned int WINAPI ThreadPool::sm_WorkerProcess(LPVOID pPTR)
	{
		custom::ThreadPool* pThreadPool = sm_pThreadPool;
		HANDLE              PrivateWorkFinishEvent = reinterpret_cast<HANDLE>(pPTR);

		SetEvent(PrivateWorkFinishEvent);

		std::function<void(void*)> Function;
		void* Parameter;

#if defined(USE_CV)
		while (1)
		{
			std::unique_lock<std::mutex> UniqueLock(pThreadPool->m_QueueMutex);
			pThreadPool->m_WakeUpCV.wait(UniqueLock, [pThreadPool]() {return pThreadPool->m_bThreadShutdown || !pThreadPool->WorkQueueEmpty(); });
			if (!pThreadPool->WorkQueueEmpty())
			{
				std::pair<std::function<void(void*)>, void*>& front = pThreadPool->m_WorkQueue.front();
				Function = front.first;
				Parameter = front.second;

				pThreadPool->m_WorkQueue.pop_front();
				++pThreadPool->m_NumWorkingThreads;
				UniqueLock.unlock();

				Function(Parameter);

				UniqueLock.lock();
				--pThreadPool->m_NumWorkingThreads;
				pThreadPool->m_WorkFinishedCV.notify_one();
			}
			else if (pThreadPool->m_bThreadShutdown)
			{
				SetEvent(PrivateWorkFinishEvent);
				return 0;
			}
		}
#else
		while (1)
		{
			pThreadPool->m_OrderMutex.lock();

			WaitForSingleObject(sm_pThreadPool->m_hWakeUpEvent, INFINITE);

			if (!sm_pThreadPool->WorkQueueEmpty())
			{
				{
					std::lock_guard<std::mutex> QueueLockguard(pThreadPool->m_QueueMutex);
					++sm_pThreadPool->m_NumWorkingThreads;

					std::pair<std::function<void(void*)>, void*>& front = pThreadPool->m_WorkQueue.front();
					Function = front.first;
					Parameter = front.second;

					pThreadPool->m_WorkQueue.pop_front();
				}

				pThreadPool->m_OrderMutex.unlock();

				Function(Parameter);

				SetEvent(PrivateWorkFinishEvent);
			}
			else
			{
				if (!pThreadPool->m_bThreadShutdown)
				{
					ResetEvent(sm_pThreadPool->m_hWakeUpEvent);
					pThreadPool->m_OrderMutex.unlock();
				}
				else
				{
					pThreadPool->m_OrderMutex.unlock();
					SetEvent(PrivateWorkFinishEvent);
					return 0;
				}
			}
		}
#endif
	}
}