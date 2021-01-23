#pragma once
namespace custom
{
	class CommandContext;
}

class IEntity;
class IWindow;
class RenderQueuePass;
class MasterRenderGraph;
class RenderingResource;

class Step
{
public:
	Step(std::string targetPassName);
	Step(Step&&) = default;
	Step(const Step&) noexcept;
	Step& operator=(const Step&) = delete;
	Step& operator=(Step&&) = delete;
	void PushBack(std::shared_ptr<RenderingResource>) noexcept;
	void Submit(const IEntity&) const;
	void Bind(custom::CommandContext& BaseContext) const DEBUG_EXCEPT;
	void InitializeParentReferences(const IEntity& _Entity) noexcept;
	void Accept(IWindow& _IWindow);
	void Link(MasterRenderGraph&);
private:
	std::vector<std::shared_ptr<RenderingResource>> m_RenderingResources;
	// std::vector<RenderingResource*> m_RenderingResources;
	RenderQueuePass* m_pTargetPass = nullptr; // BlurOutlineDrawingPass,    MainRenderPass,    OutlineDrawingPass 
										      // OutlineMaskGenerationPass, ShadowMappingPass, WireframePass
	std::string m_PassName;

	static std::mutex sm_mutex;
};