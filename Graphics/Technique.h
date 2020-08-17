#pragma once
#include "stdafx.h"
#include "Step.h"
#include "ObjectFilterFlag.cpp"

class Entity;
class ITechniqueWindow;
class MasterRenderGraph;

class Technique
{
public:
	Technique(const char* Name, eObjectFilterFlag Filter = eObjectFilterFlag::kOpaque, bool bActivating = true)
		: m_Name(Name), m_Filter(Filter), m_Steps() {}

	void Submit(const Entity& drawable, eObjectFilterFlag Filter) const noexcept;
	void PushBackStep(Step step) noexcept;
	bool IsActive() const noexcept;
	void SetActiveState(bool active_in) noexcept;
	void InitializeParentReferences(const Entity& parent) noexcept;
	void Accept(ITechniqueWindow& probe);
	const char* GetName() const noexcept;
	void Link(MasterRenderGraph&);

private:
	std::vector<Step> m_Steps;
	const char* m_Name;
	bool m_bActive = true;
	eObjectFilterFlag m_Filter;

	static std::mutex sm_mutex;
};