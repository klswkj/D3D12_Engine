#pragma once
#include "Step.h"
#include "ObjectFilterFlag.h"

class IEntity;
class ID3D12RenderQueuePass;
class IWindow;
class MasterRenderGraph;


class Technique
{
public:
	Technique(const char* szName, eObjectFilterFlag Filter = eObjectFilterFlag::kOpaque, bool bActivating = true)
		:
		m_CS({}),
		m_szTargetRenderTargetPassName(szName),
		m_Filter(Filter), 
		m_Steps(), 
		m_bActive(bActivating) 
	{
		::InitializeCriticalSection(&m_CS);
	}
	~Technique()
	{
		if (m_CS.DebugInfo)
		{
			::DeleteCriticalSection(&m_CS);
		}
	}
	void Submit(IEntity& drawable, eObjectFilterFlag Filter)noexcept;
	void PushBackStep(Step step) noexcept;
	bool IsActive() const noexcept;
	void SetActiveState(bool active_in) noexcept;
	void InitializeParentReferences(const IEntity& parent) noexcept;
	void Accept(IWindow& _IWindow);
	void Link(const MasterRenderGraph& _MasterRenderGraph);
	const char* GetTargetPassName() const noexcept;

private:
	CRITICAL_SECTION m_CS;
	const char* m_szTargetRenderTargetPassName;
	ID3D12RenderQueuePass* m_pTargetPass = nullptr; // BlurOutlineDrawingPass,    MainRenderPass,    OutlineDrawingPass 
											  // OutlineMaskGenerationPass, ShadowMappingPass, WireframePass
	eObjectFilterFlag m_Filter;
	std::vector<Step> m_Steps;
	bool m_bActive;
};