#include "stdafx.h"
#include "RenderQueuePass.h"
#include "Entity.h"
#include "Step.h"
#include "CommandContext.h"

void ID3D12RenderQueuePass::PushBackJob(IEntity& pEntity, std::vector<Step>& pSteps) //  noexcept
{
	switch (m_FactorizationStatus)
	{
	case JobFactorization::ByNone:
	{
		for (auto& IterStep : pSteps)
		{
			m_Jobs.push_back(Job{ &pEntity, &IterStep, IterStep.m_szJobName, m_FactorizationStatus });
		}
		break;
	}
	case JobFactorization::ByEntity:
	{
		m_Jobs.push_back(Job(nullptr, m_FactorizationStatus));

		Job& jobBack = m_Jobs.back();
		jobBack.m_pEntities.push_back(&pEntity);
		jobBack.m_pSteps.reserve(pSteps.size());

		for (auto& IterStep : pSteps)
		{
			jobBack.m_pSteps.push_back(&IterStep);
		}

		break;
	}
	case JobFactorization::ByStep:
	{
		for (auto& IterStep : pSteps)
		{
			// Not Allow Multiple Entity.
			bool bFindOverlappedStep = false;

			for (auto& IterJob : m_Jobs)
			{
				if (IterJob.m_szJobName == IterStep.m_szJobName)
				{
					IterJob.m_pEntities.push_back(&pEntity);
					IterJob.m_pSteps.push_back(&IterStep);
					bFindOverlappedStep = true;
					break;
				}
			}

			// TODO 2: Branch Predication Optimization
			if (!bFindOverlappedStep)
			{
				m_Jobs.push_back(Job{ &pEntity, &IterStep, IterStep.m_szJobName, m_FactorizationStatus });
			}
		}
		break;
	} // End case ByStep
	default:
		ASSERT(false, "Wrong Factorization.");
	} // End Switch Statement
}

void ID3D12RenderQueuePass::Execute(custom::CommandContext& BaseContext, uint8_t commandIndex/*= 0u*/)
{
	size_t JobSize = GetJobCount();

	if (m_bActive && JobSize)
	{
		for (const auto& _Job : m_Jobs)
		{
			_Job.RecordCommandList(BaseContext, commandIndex);
		}
	}
}

void ID3D12RenderQueuePass::ExecuteWithRange
(
	custom::CommandContext& BaseContext, uint8_t commandIndex, 
	size_t StartJobIndex/* = 0ul*/, size_t EndJobIndex/* = SIZE_MAX*/
) DEBUG_EXCEPT
{
	size_t JobSize = GetJobCount();

	if (m_bActive && JobSize)
	{
		if (JobSize <= EndJobIndex)
		{
			EndJobIndex = JobSize - 1;
		}

		ASSERT((StartJobIndex <= EndJobIndex) && 
			(EndJobIndex < JobSize));

		for (size_t i = StartJobIndex; i <= EndJobIndex; ++i)
		{
			m_Jobs[i].RecordCommandList(BaseContext, commandIndex);
		}
	}
}


void ID3D12RenderQueuePass::SetParams(custom::GraphicsContext* pGraphicsContext, uint8_t startCommandIndex, uint8_t totalCommandCount)
{
	ASSERT(pGraphicsContext);

	uint8_t CurrentParamsContainerSize = (uint8_t)m_Params.size();

	if (CurrentParamsContainerSize < totalCommandCount)
	{
		m_Params.reserve(totalCommandCount);
	}

	uint8_t CurrentCommandIndex = startCommandIndex;

	for (uint8_t i = 0; i < CurrentParamsContainerSize; ++i)
	{
		m_Params[i].pRenderQueuePass  = this;
		m_Params[i].pGraphicsContext  = pGraphicsContext;
		m_Params[i].StartCommandIndex = startCommandIndex;
		m_Params[i].CommandIndex      = CurrentCommandIndex++;
		m_Params[i].TotalCommandCount = totalCommandCount;
	}

	for (uint8_t i = CurrentParamsContainerSize; i < totalCommandCount; ++i)
	{
		m_Params.emplace_back(RenderQueueThreadParameterWrapper{ this, pGraphicsContext, startCommandIndex, CurrentCommandIndex++, totalCommandCount });
	}
}

STATIC void ID3D12RenderQueuePass::SetParamsWithVariadic(void* ptr)
{
	va_list VaList = (va_list)reinterpret_cast<va_list>(ptr);

	ID3D12RenderQueuePass* Pass = (ID3D12RenderQueuePass*)reinterpret_cast<ID3D12RenderQueuePass*>(va_arg(VaList, ID3D12RenderQueuePass*));
	custom::GraphicsContext* pGrahpicsContext = (custom::GraphicsContext*)reinterpret_cast<custom::GraphicsContext*>(va_arg(VaList, custom::GraphicsContext*));
	uint8_t StartCommandIndex = va_arg(VaList, uint8_t);
	uint8_t TotalCommandCount = va_arg(VaList, uint8_t);

	Pass->SetParams(pGrahpicsContext, StartCommandIndex, TotalCommandCount);
}

// 예외상황에 취약함
// Job의 수와 TotalCommandIndex가 나누어떨어지지 않을 때는 대비가 됐지만,
// Job의 수보다 TotalCommandIndex의 수가 더 많을 떄 방어가 아직 안됨. ex) JobCount = 1, TotalCommandIndex = 2
STATIC void ID3D12RenderQueuePass::ContextWorkerThread(LPVOID ptr)
{
	RenderQueueThreadParameterWrapper* Param = (RenderQueueThreadParameterWrapper*)reinterpret_cast<RenderQueueThreadParameterWrapper*>(ptr);
	
	ID3D12RenderQueuePass*   pRenderQueuePass  = Param->pRenderQueuePass;
	custom::GraphicsContext* pGraphicsContext  = Param->pGraphicsContext;
	uint8_t                  StartCommandIndex = Param->StartCommandIndex;
	uint8_t                  CommandIndex      = Param->CommandIndex;
	uint8_t                  TotalCommandCount = Param->TotalCommandCount;

	size_t JobCount = (pRenderQueuePass->GetJobCount() + TotalCommandCount - 1) / TotalCommandCount;

	pRenderQueuePass->::ID3D12RenderQueuePass::ExecuteWithRange
	(
		*pGraphicsContext, CommandIndex,
		JobCount * (CommandIndex - StartCommandIndex), (JobCount * ((size_t)(CommandIndex - StartCommandIndex) + 1u)) - 1
	);
}

void ID3D12RenderQueuePass::Reset() DEBUG_EXCEPT
{
	// size_t RecentlyJobs = m_Jobs.size();
	m_Jobs.clear();
	// m_Jobs.shrink_to_fit();
	// m_Jobs.reserve(RecentlyJobs);
}

size_t ID3D12RenderQueuePass::GetJobCount() const
{
	return m_Jobs.size();
}