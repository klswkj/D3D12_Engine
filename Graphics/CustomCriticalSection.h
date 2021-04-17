#pragma once
#include <cassert>
#include <synchapi.h>

namespace custom
{
	// For Global Scope.
	class RAII_CS
	{
		friend class Scoped_CS;
	public:
		explicit RAII_CS()
			:
			m_CS({})
		{
			if (!m_CS.DebugInfo)
			{
				::InitializeCriticalSection(&m_CS);
			}
		}
		~RAII_CS()
		{
			::LeaveCriticalSection(&m_CS);

			if (m_CS.DebugInfo)
			{
				::DeleteCriticalSection(&m_CS);
			}
		}

		inline void EnterCS()
		{
			::EnterCriticalSection(&m_CS);
		}
		inline void LeaveCS()
		{
			::LeaveCriticalSection(&m_CS);
		}

	private:
		::CRITICAL_SECTION m_CS;
		// =
		// PRTL_CRITICAL_SECTION_DEBUG DebugInfo;

		// LONG LockCount;
		// LONG RecursionCount;
		// HANDLE OwningThread; // from the thread's ClientId->UniqueThread
		// HANDLE LockSemaphore;
		// ULONG_PTR SpinCount;
	};

	class Scoped_CS
	{
	public:
		explicit Scoped_CS(CRITICAL_SECTION& CS)
			:
			m_pRAIICS(nullptr),
			m_pCS(CS)
		{
			if (!m_pCS.DebugInfo)
			{
				::InitializeCriticalSection(&m_pCS);
			}
			::EnterCriticalSection(&m_pCS);
		}

		explicit Scoped_CS(RAII_CS& CS)
			:
			m_pRAIICS(&CS),
			m_pCS(CS.m_CS)
		{
			if (!m_pCS.DebugInfo)
			{
				::InitializeCriticalSection(&m_pCS);
			}
			::EnterCriticalSection(&m_pCS);
		}

		~Scoped_CS()
		{
			::LeaveCriticalSection(&m_pCS);
		}
	private:
		::CRITICAL_SECTION& m_pCS;
		RAII_CS* m_pRAIICS;
	};

#pragma warning(push)
#pragma warning(disable : 26110)
	// For Global Scope.
	class RAII_SRWLock
	{
		friend class Scoped_SRWLock;
	public:
		explicit RAII_SRWLock()
			:
			m_SRWLock({})
		{
			::InitializeSRWLock(&m_SRWLock);
		}
		~RAII_SRWLock()
		{
		}

		inline void AcquireSRWLockShared()
		{
			::AcquireSRWLockShared(&m_SRWLock);
		}
		inline void AcquireSRWLockExclusive()
		{
			::AcquireSRWLockExclusive(&m_SRWLock);
		}
		inline BOOLEAN TryAcquireSRWLockShared()
		{
			return ::TryAcquireSRWLockShared(&m_SRWLock);
		}
		inline BOOLEAN TryAcquireSRWLockExclusive()
		{
			return ::TryAcquireSRWLockExclusive(&m_SRWLock);
		}

		inline void ReleaseSRWLockShared()
		{
			::ReleaseSRWLockShared(&m_SRWLock);
		}
		inline void ReleaseSRWLockExclusive()
		{
			::ReleaseSRWLockExclusive(&m_SRWLock);
		}

	private:
		::SRWLOCK m_SRWLock;
		// =
		// PVOID Ptr;
	};

	enum class eSRWScopeLockType
	{
		eShared = 0,
		eExclusive,
	};

	class Scoped_SRWLock
	{
	public:
		explicit Scoped_SRWLock(RAII_SRWLock& _RAII_SRWLock, eSRWScopeLockType lockType)
			:
			m_pSRWLock(_RAII_SRWLock.m_SRWLock),
			m_eSRWScopeLockType(lockType)
		{
			if (lockType == eSRWScopeLockType::eShared)
			{
				::AcquireSRWLockShared(&m_pSRWLock);
			}
			else
			{
				::AcquireSRWLockExclusive(&m_pSRWLock);
			}
		}

		explicit Scoped_SRWLock(::SRWLOCK& _SRWLock, eSRWScopeLockType lockType)
			:
			m_pSRWLock(_SRWLock),
			m_eSRWScopeLockType(lockType)
		{
			if (lockType == eSRWScopeLockType::eShared)
			{
				::AcquireSRWLockShared(&m_pSRWLock);
			}
			else
			{
				::AcquireSRWLockExclusive(&m_pSRWLock);
			}
		}

		void SwitchLock()
		{
			if (m_eSRWScopeLockType == eSRWScopeLockType::eShared)
			{
				::ReleaseSRWLockShared(&m_pSRWLock);
				::AcquireSRWLockExclusive(&m_pSRWLock);
				m_eSRWScopeLockType = eSRWScopeLockType::eExclusive;
			}
			else
			{
				::ReleaseSRWLockExclusive(&m_pSRWLock);
				::AcquireSRWLockShared(&m_pSRWLock);
				m_eSRWScopeLockType = eSRWScopeLockType::eShared;
			}
		}

		~Scoped_SRWLock()
		{
			if (m_eSRWScopeLockType == eSRWScopeLockType::eShared)
			{
				::ReleaseSRWLockShared(&m_pSRWLock);
			}
			else
			{
				::ReleaseSRWLockExclusive(&m_pSRWLock);
			}
		}
	private:
		::SRWLOCK&        m_pSRWLock;
		eSRWScopeLockType m_eSRWScopeLockType;
	};
#pragma warning(pop)
}