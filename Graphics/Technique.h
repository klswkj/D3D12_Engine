#pragma once
#include "Step.h"
#include "ObjectFilterFlag.h"

class Entity;
class ITechniqueWindow;
class MasterRenderGraph;

class Technique
{
public:
	Technique(std::string Name, eObjectFilterFlag Filter = eObjectFilterFlag::kOpaque, bool bActivating = true)
		: m_Name(Name), m_Filter(Filter), m_Steps(), m_bActive(bActivating) {}

	void Submit(const Entity& drawable, eObjectFilterFlag Filter) const noexcept;
	void PushBackStep(Step step) noexcept;
	bool IsActive() const noexcept;
	void SetActiveState(bool active_in) noexcept;
	void InitializeParentReferences(const Entity& parent) noexcept;
	void Accept(ITechniqueWindow& probe);
	void Link(MasterRenderGraph& _MasterRenderGraph);
	std::string GetName() const noexcept;

private:
	std::vector<Step> m_Steps;
	std::string m_Name;
	bool m_bActive;
	eObjectFilterFlag m_Filter;

	static std::mutex sm_mutex;
};