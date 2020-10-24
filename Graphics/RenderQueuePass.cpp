#include "stdafx.h"
#include "RenderQueuePass.h"

void RenderQueuePass::PushBackJob(Job _Job) noexcept
{
	m_Jobs.push_back(_Job); // Job -> { pEntity, pStep } -> { pEntity, {vector<RenderingResoruce>} }
}

void RenderQueuePass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	if (m_bActive)
	{
		IBindingPass::bindAll(BaseContext);

		for (const auto& _Job : m_Jobs)
		{
			_Job.Execute(BaseContext);
		}
	}
}
void RenderQueuePass::Reset() DEBUG_EXCEPT
{
	m_Jobs.clear();
}