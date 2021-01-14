#include "stdafx.h"
#include "Job.h"
#include "CommandContext.h"

#include "ComputeContext.h"
#include "Entity.h"
#include "Step.h"

void Job::Execute(custom::CommandContext& BaseContext) const DEBUG_EXCEPT
{
	m_pEntity->Bind(BaseContext);
	m_pStep->Bind(BaseContext); // RootSignature��, PSO ���ε��� ��, ����ȭ Ȯ��

	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.DrawIndexed(m_pEntity->GetIndexCount(), m_pEntity->GetStartIndexLocation(), m_pEntity->GetBaseVertexLocation());
}