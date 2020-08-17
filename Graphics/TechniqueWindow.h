#pragma once

class Technique;

class ITechniqueWindow
{
public:
	// TODO: add callback for visiting each mesh
	virtual ~ITechniqueWindow()
	{
	}
	void SetTechnique(Technique* pTech_in)
	{
		m_pTechnique = pTech_in;
		m_TechniqueIndex++;
		OnSetTechnique();
	}
	void SetStep(Step* pStep_in)
	{
		m_pStep = pStep_in;
		m_StepIndex++;
		OnSetStep();
	}
	bool VisitBuffer(/*Dcb::Buffer& buf*/)
	{
		m_BufferIndex++;
		return false;
	}
protected:
	virtual void OnSetTechnique() = 0;
	virtual void OnSetStep() = 0;
	virtual bool OnVisitBuffer(/*Dcb::Buffer&*/) = 0;

protected:
	Step* m_pStep           { nullptr };
	Technique* m_pTechnique { nullptr };

	size_t m_TechniqueIndex { -1 };
	size_t m_StepIndex      { -1 };
	size_t m_BufferIndex    { -1 };
};