#pragma once
#include "stdafx.h"

class RenderingResource;

namespace custom
{
	class CommandContext;
}

class Entity;
class ITechniqueWindow;
class RenderQueuePass;
class MasterRenderGraph;

class Step
{
public:
	Step(const char* targetPassName);
	Step(Step&&) = default;
	Step(const Step&) noexcept;
	Step& operator=(const Step&) = delete;
	Step& operator=(Step&&) = delete;
	void PushBack(std::shared_ptr<RenderingResource>) noexcept;
	void Submit(const Entity&) const;
	void Bind(custom::CommandContext&) const DEBUG_EXCEPT;
	void InitializeParentReferences(const Entity&) noexcept;
	void Accept(ITechniqueWindow&);
	void Link(MasterRenderGraph&);
private:
	std::vector<std::shared_ptr<RenderingResource>> m_RenderingResources;
	// std::vector<RenderingResource*> m_RenderingResources;
	RenderQueuePass* m_pTargetPass = nullptr; // BlurOutlineDrawingPass,    LambertianPass,    OutlineDrawingPass 
										      // OutlineMaskGenerationPass, ShadowMappingPass, WireframePass
	const char* m_TargetPassName;

	static std::mutex sm_mutex;
};