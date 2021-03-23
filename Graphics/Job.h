#pragma once
#include "Entity.h"
#include "Step.h"

namespace custom
{
	class CommandContext;
}

enum class JobFactorization
{
	ByNone   = 0,
	ByEntity = 1,
	ByStep   = 2
};

class Job
{
	friend class ID3D12RenderQueuePass;
	friend class MasterRenderGraph;
public:
	explicit Job(const char* szName, const JobFactorization status)
		:
		m_szJobName(szName),
		m_FactorizationStatus(status)
#if defined(_DEBUG)
		, m_bDrawCompleted(true)
#endif
	{
	};
	explicit Job(IEntity* const pEntity, Step* const pStep, const char* szName, const JobFactorization status)
		:
		m_szJobName(szName),
		m_FactorizationStatus(status)
#if defined(_DEBUG)
		, m_bDrawCompleted(true)
#endif
	{
		m_pEntities.push_back(pEntity);
		m_pSteps.push_back(pStep);
	}

	~Job()
	{
#if defined(_DEBUG)
		ASSERT(m_bDrawCompleted);
#endif
	}

	void InsertEntity(IEntity* const pEntity) { m_pEntities.push_back(pEntity); }
	void InsertEntity(Step* const pStep)      { m_pSteps.push_back(pStep); }

	void RecordCommandList(custom::CommandContext& BaseContext, uint8_t commandIndex) const DEBUG_EXCEPT;

#if defined (_DEBUG)
public:
	mutable bool m_bDrawCompleted;
#endif

private:
	const char* const      m_szJobName;
	const JobFactorization m_FactorizationStatus;
	std::vector<IEntity*> m_pEntities;
	std::vector<Step*>    m_pSteps;

};