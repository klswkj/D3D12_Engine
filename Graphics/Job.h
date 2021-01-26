#pragma once
#include "Entity.h"
#include "Step.h"

namespace custom
{
	class CommandContext;
}

class Job
{
public:
	Job(const IEntity* Entity, const Step* _Step)
		: m_pEntity{ Entity }, m_pStep{ _Step }
	{
	}
	void Execute(custom::CommandContext& BaseContext) const DEBUG_EXCEPT;

private:
	const IEntity* m_pEntity;
	const Step*    m_pStep;
};