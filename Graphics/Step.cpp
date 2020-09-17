#include "stdafx.h"
#include "Step.h"
#include "CommandContext.h"

#include "Job.h"
#include "TechniqueWindow.h"
#include "RenderingResource.h"
#include "RenderQueuePass.h"
#include "MasterRenderGraph.h"

std::mutex Step::sm_mutex;

Step::Step(const char* PassName)
	: m_PassName(PassName) 
{
}

Step::Step(const Step& other) noexcept
{
	m_RenderingResources.reserve(other.m_RenderingResources.size());

	std::lock_guard<std::mutex> LockGuard(sm_mutex);

	for (auto& e : other.m_RenderingResources)
	{
		m_RenderingResources.push_back(e);
	}
}

void Step::PushBack(std::shared_ptr<RenderingResource> _RenderingResource) noexcept
{
	std::lock_guard<std::mutex> LockGuard(sm_mutex);

	m_RenderingResources.push_back(_RenderingResource);
}
void Step::Submit(const Entity& _Entity) const
{
	m_pPass->PushBackJob(Job{ &_Entity, this });
}
void Step::Bind(custom::CommandContext& BaseContext) const DEBUG_EXCEPT
{
	for (const auto& e : m_RenderingResources)
	{
		e->Bind(BaseContext);
	}
}

void Step::InitializeParentReferences(const Entity & parent) noexcept
{
	for (const auto& e : m_RenderingResources)
	{
		// e->InitializeParentReference(parent);
	}
}
void Step::Accept(ITechniqueWindow& probe)
{
	probe.SetStep(this);
}

void Step::Link(MasterRenderGraph& _MasterRenderGraph)
{
	ASSERT(m_pPass == nullptr);
	m_pPass = &_MasterRenderGraph.GetRenderQueue(m_PassName);
}