#pragma once
#include "Entity.h"
#include "Step.h"

namespace custom
{
	class CommandContext;
}

enum class JobFactorization
{
	ByNone = 0,
	ByEntity = 1,
	ByStep = 2
};

class Job
{
	friend class ID3D12RenderQueuePass;
public:
	explicit Job(const char* szName, const JobFactorization status)
		:
		m_szJobName(szName),
		m_FactorizationStatus(status)
	{
	};
	explicit Job(IEntity* const pEntity, Step* const pStep, const char* szName, const JobFactorization status)
		:
		m_szJobName(szName),
		m_FactorizationStatus(status)
	{
		m_pEntities.push_back(pEntity);
		m_pSteps.push_back(pStep);
	}

	void InsertEntity(IEntity* const pEntity) { m_pEntities.push_back(pEntity); }
	void InsertEntity(Step* const pStep) { m_pSteps.push_back(pStep); }

	void RecordCommandList(custom::CommandContext& BaseContext, uint8_t commandIndex) const DEBUG_EXCEPT;

private:
	const char* const      m_szJobName;
	const JobFactorization m_FactorizationStatus;
	std::vector<IEntity*> m_pEntities;
	std::vector<Step*>    m_pSteps;
};