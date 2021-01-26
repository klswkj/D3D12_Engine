#pragma once
#include "Pass.h"
#include "Job.h"

namespace custom
{
	class CommandContext;
}

class RenderingResource;
class PixelBuffer;
class RenderTarget; // Will be deleted.
class DepthStencil; // Will be deleted.

// TODO 3 : ���߿� sequential Populate RenderQueuePass ����
// Z-Sorting�̳�(���ĺ���), Cascade Shadow Mappingó�� ����������
// �����ؾ� �� ���� ����ؼ� �̸��̸� �����س��ߵɵ�.


class RenderQueuePass : public Pass
{
public:
	// Be Inherited all constructors from Base class (BindingPass).
	// -> Be Inherited all constructors directly from Pass class.
	RenderQueuePass(std::string Name) : Pass(Name) {}

	void PushBackJob(Job _Job) noexcept;
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT; // Add size_t StartStepIndex, size_t EndStepIndex
	void ExecuteWithRange(custom::CommandContext& BaseContext, size_t StartStepIndex = 0ul, size_t EndStepIndex = SIZE_MAX);
	void Reset() DEBUG_EXCEPT override;
	size_t GetJobCount() const;

private:
	std::vector<Job> m_Jobs;
};