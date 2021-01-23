#include "stdafx.h"
#include "Technique.h"
#include "Entity.h"
#include "ModelComponentWindow.h"
#include "ObjectFilterFlag.h"

std::mutex Technique::sm_mutex;

void Technique::Submit(const IEntity& drawable, eObjectFilterFlag Filter) const noexcept
{
	if (m_bActive && ((m_Filter && Filter) != 0))
	{
		for (const auto& Step : m_Steps)
		{
			Step.Submit(drawable);
		}
	}
}

void Technique::InitializeParentReferences(const IEntity& parent) noexcept
{
	for (auto& Step : m_Steps)
	{
		Step.InitializeParentReferences(parent);
	}
}

void Technique::PushBackStep(Step _Step) noexcept
{
	std::lock_guard<std::mutex> LockGuard(sm_mutex);
	m_Steps.push_back(std::move(_Step));
}

bool Technique::IsActive() const noexcept
{
	return m_bActive;
}

void Technique::SetActiveState(bool Active) noexcept
{
	m_bActive = Active;
}

void Technique::Accept(IWindow& _IWindow)
{
	_IWindow.SetTechnique(this);

	for (auto& Step : m_Steps)
	{
		Step.Accept(_IWindow);
	}
}

void Technique::Link(MasterRenderGraph& _MasterRenderGraph)
{
	for (auto& Step : m_Steps)
	{
		Step.Link(_MasterRenderGraph);
	}
}

std::string Technique::GetName() const noexcept
{
	return m_Name;
}
