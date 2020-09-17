#include "stdafx.h"
#include "Job.h"
#include "CommandContext.h"

#include "ComputeContext.h"
#include "Entity.h"
#include "Step.h"

void Job::Execute(custom::CommandContext& BaseContext) const DEBUG_EXCEPT
{
	m_pEntity->Bind(BaseContext);
	m_pStep->Bind(BaseContext);
	BaseContext.GetGraphicsContext().DrawIndexed(m_pEntity->GetIndexCount());
	// BaseContext.GetComputeContext().DispatchIndirect;
}