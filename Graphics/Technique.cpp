#include "stdafx.h"
#include "Technique.h"
#include "MasterRenderGraph.h"
#include "Entity.h"
#include "RenderQueuePass.h"
#include "ModelComponentWindow.h"
#include "ObjectFilterFlag.h"

void Technique::Submit(IEntity& drawable, eObjectFilterFlag Filter) noexcept
{
	if (m_bActive && ((m_Filter && Filter) != 0))
	{
		m_pTargetPass->PushBackJob(drawable, m_Steps);
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
	::EnterCriticalSection(&m_CS);
	m_Steps.push_back(std::move(_Step));
	::LeaveCriticalSection(&m_CS);
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

void Technique::Link(const MasterRenderGraph& _MasterRenderGraph)
{
	m_pTargetPass = _MasterRenderGraph.FindRenderQueuePass(m_szTargetRenderTargetPassName);
}

const char* Technique::GetTargetPassName() const noexcept
{
	return m_szTargetRenderTargetPassName;
}
