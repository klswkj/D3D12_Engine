#include "stdafx.h"
#include "RenderQueuePass.h"

void RenderQueuePass::PushBackJob(Job _Job) noexcept
{
	m_Jobs.push_back(_Job); // Job -> { pEntity, pStep } -> { pEntity, {vector<RenderingResoruce>} }
}

void RenderQueuePass::Execute(custom::CommandContext& BaseContext)
{
	size_t JobSize = GetJobCount();

	if (m_bActive && JobSize)
	{
		for (const auto& _Job : m_Jobs)
		{
			_Job.Execute(BaseContext);
		}
	}
}

void RenderQueuePass::ExecuteWithRange(custom::CommandContext& BaseContext, size_t StartStepIndex/* = 0ul*/, size_t EndStepIndex/* = SIZE_MAX*/) DEBUG_EXCEPT
{
	size_t JobSize = GetJobCount();

	if (m_bActive && JobSize)
	{
		// IBindingPass::bindAll(BaseContext);

		if ((StartStepIndex == 0ul) &&
			(EndStepIndex == SIZE_MAX))
		{
			for (const auto& _Job : m_Jobs)
			{
				_Job.Execute(BaseContext);
			}
		}
		else
		{
			ASSERT(StartStepIndex <= EndStepIndex);
			ASSERT(EndStepIndex < JobSize);

			for (size_t i = StartStepIndex; i <= EndStepIndex; ++i)
			{
				m_Jobs[i].Execute(BaseContext);
			}
		}
	}
}
void RenderQueuePass::Reset() DEBUG_EXCEPT
{
	m_Jobs.clear();
}

size_t RenderQueuePass::GetJobCount() const
{
	return m_Jobs.size();
}