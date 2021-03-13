#include "stdafx.h"
#include "Job.h"
#include "CommandContext.h"

#include "ComputeContext.h"
#include "Entity.h"
#include "Step.h"

void Job::RecordCommandList(custom::CommandContext& BaseContext, uint8_t commandIndex) const DEBUG_EXCEPT
{
	custom::GraphicsContext& GC = BaseContext.GetGraphicsContext();

	switch (m_FactorizationStatus)
	{
	case JobFactorization::ByNone:
	{
		ASSERT((m_pEntities.size() == 1) && (m_pSteps.size() && 1))
		IEntity* pEntity = m_pEntities.front();

		pEntity->Bind(BaseContext, commandIndex);
		m_pSteps.front()->Bind(BaseContext, commandIndex);
		GC.DrawIndexed(pEntity->m_IndexCount, pEntity->m_StartIndexLocation, pEntity->m_BaseVertexLocation, commandIndex);
		break;
	}
	case JobFactorization::ByEntity:
	{
		ASSERT(m_pEntities.size() == 1);
		IEntity* pEntity = m_pEntities.front();
		pEntity->Bind(BaseContext, commandIndex);

		for (const auto& IterStep : m_pSteps)
		{
			IterStep->Bind(BaseContext, commandIndex);
			GC.DrawIndexed(pEntity->m_IndexCount, pEntity->m_StartIndexLocation, pEntity->m_BaseVertexLocation, commandIndex);
		}

		break;
	}
	case JobFactorization::ByStep:
	{
		size_t JobSize = m_pEntities.size();
		ASSERT(JobSize == m_pSteps.size());

		for (size_t i = 0; i < JobSize; ++i)
		{
			m_pEntities[i]->Bind(BaseContext, commandIndex);
			m_pSteps[i]->Bind(BaseContext, commandIndex);
			GC.DrawIndexed(m_pEntities[i]->m_IndexCount, m_pEntities[i]->m_StartIndexLocation, m_pEntities[i]->m_BaseVertexLocation, commandIndex);
		}

		break;
	} // End case ByStep
	default:
		ASSERT(false, "Wrong Factorization.");
	}
}

/*
void Job::RecordCommandList(custom::CommandContext& BaseContext, uint8_t commandIndex) const DEBUG_EXCEPT
{
	m_pEntity->Bind(BaseContext, commandIndex);
	m_pStep->Bind(BaseContext, commandIndex);

	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

#if defined(_DEBUG)
	graphicsContext.PIXBeginEvent(StringToWString(m_pEntity->m_name).c_str(), commandIndex);
	graphicsContext.DrawIndexed(m_pEntity->GetIndexCount(), m_pEntity->GetStartIndexLocation(), m_pEntity->GetBaseVertexLocation(), commandIndex);
	graphicsContext.PIXEndEvent(commandIndex);
#else
	graphicsContext.DrawIndexed(m_pEntity->GetIndexCount(), m_pEntity->GetStartIndexLocation(), m_pEntity->GetBaseVertexLocation(), commandIndex);
#endif
}
*/