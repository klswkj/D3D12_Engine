#pragma once
#include "Pass.h"
#include "BindingPass.h"
#include "Job.h"

namespace custom
{
	class CommandContext;
}

class RenderingResource;
class RenderTarget;
class DepthStencil;

class RenderQueuePass : public IBindingPass
{
public:
	// Inherit all constructors from Base class (BindingPass).
	RenderQueuePass(std::string Name) : IBindingPass(Name) {}

	void PushBackJob(Job _Job) noexcept;
	void Execute(custom::CommandContext&) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;
private:
	std::vector<Job> m_Jobs;
};