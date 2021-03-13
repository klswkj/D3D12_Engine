#include "stdafx.h"
#include "Step.h"
#include "CommandContext.h"

#include "ModelComponentWindow.h"
#include "Job.h"
#include "RenderingResource.h"
#include "MasterRenderGraph.h"

Step::Step(const char* szJobName)
	:
	m_szJobName(szJobName)
{
}

Step::Step(const Step& other) noexcept
	: 
	m_szJobName(other.m_szJobName)
{
	m_RenderingResources.reserve(other.m_RenderingResources.size());

	for (auto& e : other.m_RenderingResources)
	{
		m_RenderingResources.push_back(e);
	}
}

void Step::PushBack(std::shared_ptr<RenderingResource> _RenderingResource) noexcept
{
	m_RenderingResources.push_back(std::move(_RenderingResource));\
}
// void Step::Submit(const IEntity& _Entity) const
// {
	// m_pTargetPass->PushBackJob(Job{ &_Entity, this });
// }
void Step::Bind(custom::CommandContext& BaseContext, uint8_t commandIndex) const DEBUG_EXCEPT
{
	for (const auto& e : m_RenderingResources)
	{
		e->Bind(BaseContext, commandIndex);
	}
}

void Step::InitializeParentReferences(const IEntity & parent) noexcept
{
	for (const auto& e : m_RenderingResources)
	{
		e->InitializeParentReference(parent);
	}
}
void Step::Accept(IWindow& _IWindow)
{
	_IWindow.SetStep(this);

	for (auto& _RR : m_RenderingResources)
	{
		_RR->RenderingWindow(_IWindow);
	}
}