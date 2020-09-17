#pragma once
namespace custom
{
	class CommandContext;
}

class Entity;
class ITechniqueWindow;
class RenderQueuePass;
class MasterRenderGraph;
class RenderingResource;

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
	void Bind(custom::CommandContext& BaseContext) const DEBUG_EXCEPT;
	void InitializeParentReferences(const Entity& _Entity) noexcept;
	void Accept(ITechniqueWindow&);
	void Link(MasterRenderGraph&);
private:
	std::vector<std::shared_ptr<RenderingResource>> m_RenderingResources;
	// std::vector<RenderingResource*> m_RenderingResources;
	RenderQueuePass* m_pPass = nullptr; // BlurOutlineDrawingPass,    MainRenderPass,    OutlineDrawingPass 
										      // OutlineMaskGenerationPass, ShadowMappingPass, WireframePass
	const char* m_PassName;

	static std::mutex sm_mutex;
};