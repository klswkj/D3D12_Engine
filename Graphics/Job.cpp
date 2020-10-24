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
	// TODO : make 
	// BaseContext.GetGraphicsContext().DrawIndexed(m_pEntity->GetIndexCount(), m_pEntity->GetStartIndexLocation(), m_pEntity->GetBaseVertexLocation());
	
	
	// BaseContext.GetComputeContext().DispatchIndirect;
}

// Entity에 IndexCount, StartLocation, BaseVertexLocation 추가해야됨.