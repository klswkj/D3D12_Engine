#pragma once
#include "CustomCriticalSection.h"

namespace custom
{
	// My Computer Spec, 
	// Not Using CV Mode : Average about x4.0 time faster.
	// Using CV Mode     : Average about x3.5 time faster.

	class ThreadPool
	{
		friend class Thread;
	public:
		ThreadPool();
		~ThreadPool();

		bool Create(uint32_t NumThreads = 0);
		void AddThreads(uint32_t NumAddThreads);
		void Resize(uint32_t NumNewThreads);
		void ThreadsShutdown();
		bool WorkQueueEmpty() { Scoped_CS Scoped(m_QueueCS); return m_WorkQueue.empty(); }

	public:
		static void Enqueue
		(
			_In_ std::function<void(void*)> pWork,
			_In_opt_ void* pParameter = nullptr
		);

		static void Enqueue
		(
			_In_ std::function<void(void*)>&& pWork,
			_In_opt_ void*&& pParameter = nullptr
		);

		static void EnqueueVariadic(_In_ std::function<void(void*)>&& pWork, int NumParam, ...);

		static void MultipleEnqueue
		(
			_In_ std::function<void(void*)> pWorks[],
			_In_ size_t numElements,
			_In_opt_ void* pParameters[] = nullptr,
			_In_opt_ size_t ElementSize = 0ull
		);

		static void MultipleEnqueue
		(
			_In_ std::function<void(void*)> pWork,
			_In_ size_t numElements,
			_In_opt_ void* pParameters[] = nullptr,
			_In_opt_ size_t ElementSize = 0ull
		);

		// 버그 : WorkThread한테 모두 일을 줘야지 나올수가 있음...
		static void   WaitAllFinished(_In_opt_ HANDLE hFinishEvent = nullptr);
		static size_t GetLogicalProcessors() { return ThreadPool::sm_LogicalProcessors; }

	private:
		void WaitFinishedAllThreads()
		{
#ifdef _DEBUG
			uint64_t WaitFailed = 0ull;
#endif
			while (1)
			{
				if (WAIT_TIMEOUT == WaitForMultipleObjects((DWORD)m_ThreadWorkFinishEvents.size(), m_ThreadWorkFinishEvents.data(), false, 0))
				{
					break;
				}
#ifdef _DEBUG
				else
				{
					++WaitFailed;
				}
#endif
			}

#ifdef _DEBUG
			printf("Thread Pool's First Wait Failed : #%d\n", WaitFailed);
#endif

			uint32_t NumRemaindWorkingThread = InterlockedGetValue(&m_NumWorkingThreads);
			DWORD WaitingResult = WAIT_OBJECT_0;

			while (NumRemaindWorkingThread)
			{
				WaitingResult = ::WaitForMultipleObjects((DWORD)m_ThreadWorkFinishEvents.size(), m_ThreadWorkFinishEvents.data(), false, INFINITE);
				if (WaitingResult < 0x80L)
				{
					--NumRemaindWorkingThread;
				}
#ifdef _DEBUG
				else
				{
					++WaitFailed;
				}
#endif
			}

#ifdef _DEBUG
			printf("Thread Pool's Last Wait Failed : #%d\n", WaitFailed);
			printf("m_NumWorkingThreads : %d\n", InterlockedGetValue(&m_NumWorkingThreads));
#endif
			if (InterlockedGetValue(&m_NumWorkingThreads))
			{
				WaitFinishedAllThreads();
			}
		}

	private:
		static ThreadPool* sm_pThreadPool;
		static size_t      sm_LogicalProcessors;

		std::vector<HANDLE> m_Threads;
		std::vector<HANDLE> m_ThreadWorkFinishEvents;
		std::deque<std::pair<std::function<void(void*)>, void*>> m_WorkQueue;

		CRITICAL_SECTION m_QueueCS;
		CRITICAL_SECTION m_OrderCS;
		HANDLE m_hWakeUpEvent;
		HANDLE m_hWorkFinishEvent;

		BOOL m_bThreadShutdown;

		// TODO 1: 메모리 버스 많이 잠겨서 SRWLock으로 바꿔야함.
		uint32_t volatile m_NumWorkingThreads;

		static unsigned int WINAPI sm_WorkerProcess(LPVOID pThreadPool);
	};
}


/*
void WaitFinishedAllThreads()
		{
while (WAIT_OBJECT_0 != ::WaitForSingleObject(m_hWorkFinishEvent, 0))
			{
				if (!InterlockedGetValue(&m_NumWorkingThreads))
				{
					break;
				}
			}

			::ResetEvent(m_hWorkFinishEvent);
		}
*/