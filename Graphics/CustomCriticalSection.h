#pragma once
#include <cassert>
#include <synchapi.h>

namespace custom
{
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

		void EnterCS()
		{
			::EnterCriticalSection(&m_CS);
		}
		void LeaveCS()
		{
			::LeaveCriticalSection(&m_CS);
		}

	private:
		CRITICAL_SECTION m_CS;
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
		CRITICAL_SECTION& m_pCS;
		RAII_CS* m_pRAIICS;
	};
}