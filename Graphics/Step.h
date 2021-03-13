#pragma once
namespace custom
{
	class CommandContext;
}

class IEntity;
class IWindow;
class MasterRenderGraph;
class RenderingResource;

class Step
{
	friend class ID3D12RenderQueuePass;
public:
	Step(const char* szRenderQueuePassKey);
	Step(Step&&) = default;
	Step(const Step&) noexcept;
	Step& operator=(const Step&) = delete;
	Step& operator=(Step&&) = delete;
	~Step() = default;
	void PushBack(std::shared_ptr<RenderingResource>) noexcept;
	// void Submit(const IEntity&) const;
	void Bind(custom::CommandContext& baseContext, uint8_t commandIndex) const DEBUG_EXCEPT;
	void InitializeParentReferences(const IEntity& _Entity) noexcept;
	void Accept(IWindow& _IWindow);
private:
	const char* const m_szJobName;
	std::vector<std::shared_ptr<RenderingResource>> m_RenderingResources;
};