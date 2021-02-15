#pragma once

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
		bool WorkQueueEmpty() { return m_WorkQueue.empty(); }

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

		static void MultipleEnqueue
		(
			_In_ std::function<void(void*)> pWork[],
			_In_ size_t ElementSize,
			_In_opt_ void* pParameter[] = nullptr
		);

		static void   WaitAllFinished(_In_opt_ HANDLE hFinishEvent = nullptr);
		static size_t GetLogicalProcessors() { return ThreadPool::sm_LogicalProcessors; }

	private:
		void WaitFinishedAllThreads()
		{
			DWORD Result = WaitForMultipleObjects((DWORD)m_ThreadWorkFinishEvents.size(), m_ThreadWorkFinishEvents.data(), true, INFINITE);

			if (Result == WAIT_OBJECT_0)
			{
				return;
			}
			else
			{
				DWORD LastError = GetLastError();
				ASSERT(!LastError);
			}
		}

	private:
		static ThreadPool* sm_pThreadPool;
		static size_t sm_LogicalProcessors;

		std::vector<HANDLE> m_Threads;
		std::vector<HANDLE> m_ThreadWorkFinishEvents;
		std::deque<std::pair<std::function<void(void*)>, void*>> m_WorkQueue;

		CRITICAL_SECTION m_QueueCS;
		CRITICAL_SECTION m_OrderCS;
		HANDLE     m_hWakeUpEvent;
		HANDLE     m_hWorkFinishEvent;

		BOOL       m_bThreadShutdown;
		uint32_t   m_NumWorkingThreads;

		static unsigned int WINAPI sm_WorkerProcess(LPVOID pThreadPool);
	};
}
