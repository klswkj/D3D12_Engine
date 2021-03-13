#pragma once
#include "Pass.h"
#include "Job.h"

namespace custom
{
	class CommandContext;
}

class IEntity;
class Step;

class ID3D12RenderQueuePass : public ID3D12MTPass
{
public:
	explicit ID3D12RenderQueuePass(std::string Name, JobFactorization status)
		: 
		ID3D12MTPass(Name),
		m_FactorizationStatus(status)
	{
	}
	virtual ~ID3D12RenderQueuePass() = default;

	void PushBackJob(IEntity& pEntity, std::vector<Step>& pSteps);//  noexcept;
	void Execute(custom::CommandContext& BaseContext, uint8_t commandIndex) DEBUG_EXCEPT;
	void ExecuteWithRange(custom::CommandContext& BaseContext, uint8_t commandIndex, size_t StartJobIndex = 0ul, size_t EndJobIndex = SIZE_MAX);

	void SetParams(custom::GraphicsContext* pGraphicsContext, uint8_t startCommandIndex, uint8_t totalCommandCount);
	void Reset() DEBUG_EXCEPT override;
	size_t GetJobCount() const;

public:
	struct RenderQueueThreadParameterWrapper
	{
		ID3D12RenderQueuePass*   pRenderQueuePass;
		custom::GraphicsContext* pGraphicsContext;
		uint8_t                  StartCommandIndex;
		uint8_t                  CommandIndex;      // ÇÒ´çµÈ CL Index
		uint8_t                  TotalCommandCount; // ÃÑ CL°¹¼ö, 
	};

	static void ContextWorkerThread(LPVOID ptr);
	static void SetParamsWithVariadic(void* ptr);

protected:
	std::vector<Job> m_Jobs;
	std::vector<RenderQueueThreadParameterWrapper> m_Params;
	const JobFactorization m_FactorizationStatus;
};