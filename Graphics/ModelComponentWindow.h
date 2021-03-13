#pragma once
#include <limits>
#include <string>
#include "ModelPart.h"

class Model;
class Technique;
class Step;

class IWindow
{
public:
	virtual ~IWindow() = default;

	void SetTechnique(Technique* pTech_in)
	{
		m_pTech = pTech_in;
		m_TechIndex++;
		OnSetTechnique();
	}
	void SetStep(Step* pStep_in)
	{
		m_pStep = pStep_in;
		m_StepIndex++;
		OnSetStep();
	}

protected:
	virtual void OnSetTechnique() {}
	virtual void OnSetStep() {}

protected:
	Technique* m_pTech     = nullptr;
	Step*      m_pStep     = nullptr;
	size_t     m_TechIndex = ULLONG_MAX;
	size_t     m_StepIndex = ULLONG_MAX;
};

class MaterialWindow : public IWindow
{
	void OnSetTechnique() override
	{
		ImGui::TextColored({0.4f, 1.0f, 0.6f, 1.0f}, m_pTech->GetTargetPassName());
		bool bActive = m_pTech->IsActive();
		ImGui::Checkbox((std::string("Tech Active ") + std::to_string(m_TechIndex)).c_str(), &bActive);
		m_pTech->SetActiveState(bActive);
	}
};

class ModelComponentWindow : public IWindow
{
public:
	ModelComponentWindow(std::string _Name)
		: m_Name(std::move(_Name))
	{
	}
	void ShowWindow(Model& _Model);

	bool PushNode(ModelPart& _ModelPart);
	void PopNode(ModelPart&);
	struct TransformParameters
	{
		float xRot = 0.0f;
		float yRot = 0.0f;
		float zRot = 0.0f;
		float x    = 0.0f;
		float y    = 0.0f;
		float z    = 0.0f;
	};
private:
	TransformParameters& resolveTransform() noexcept;
	TransformParameters& loadTransform(uint32_t _Key);

private:
	ModelPart* m_pSelectedModelPart = nullptr;

	std::string m_Name;
	std::unordered_map<uint32_t, TransformParameters> m_TransformParameter;
};