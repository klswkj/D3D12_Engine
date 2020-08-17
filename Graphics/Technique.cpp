#include "Technique.h"
#include "Entity.h"
#include "TechniqueWindow.h"
#include "ObjectFilterFlag.h"

std::mutex Technique::sm_mutex;

void Technique::Submit(const Entity& drawable, eObjectFilterFlag Filter) const noexcept
{
	if (m_bActive && ((m_Filter && Filter) != 0))
	{
		for (const auto& Step : m_Steps)
		{
			Step.Submit(drawable);
		}
	}
}

void Technique::InitializeParentReferences(const Entity& parent) noexcept
{
	for (auto& Step : m_Steps)
	{
		Step.InitializeParentReferences(parent);
	}
}

void Technique::PushBackStep(Step _Step)
{
	std::lock_guard<std::mutex> LockGuard(sm_mutex);
	m_Steps.push_back(std::move(_Step));
}

bool Technique::IsActive() const noexcept
{
	return m_bActive;
}

void Technique::SetActiveState(bool Active)
{
	m_bActive = Active;
}

void Technique::Accept(ITechniqueWindow& Window)
{
	Window.SetTechnique(this);
	for (auto& Step : m_Steps)
	{
		Step.Accept(Window);
	}
}

const char* Technique::GetName() const
{
	return m_Name;
}

void Technique::Link(MasterRenderGraph& _MasterRenderGraph)
{
	for (auto& Step : m_Steps)
	{
		Step.Link(_MasterRenderGraph);
	}
}