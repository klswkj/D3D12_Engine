#pragma once
#include "Model.h"

class RenderQueuePass;
class Camera;
class ShadowCamera;
class Source;
class Sink;
class Pass;
class RenderTarget;
class DepthStencil;
enum class eObjectFilter;

namespace custom
{
	class CommandContext;
}

namespace Math
{
	class Matrix4;
}

class MasterRenderGraph
{
public:
	MasterRenderGraph();
	~MasterRenderGraph();
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void Reset() noexcept;
	RenderQueuePass& GetRenderQueue(const std::string& passName);

	void ShowPassesWindows();
	void BindMainCamera(Camera& cam);
	void BindShadowCamera(Camera& cam);

private:
	void finalize();
	void appendPass(std::unique_ptr<Pass> pass);
	Pass& findPassByName(const std::string& name);

public:
	std::vector<std::unique_ptr<Pass>>   m_pPasses;
	Camera* m_CurrentActiveCamera{ nullptr };

private:
	std::vector<ShadowCamera> m_SunShadows; // Must have at least one SunShadow.
	std::vector<Model> m_Models;

	GraphicsPSO m_DepthPSO;
	GraphicsPSO m_CutoutDepthPSO;
	GraphicsPSO m_ShadowPSO;
	GraphicsPSO m_CutoutShadowPSO;
	GraphicsPSO m_CountingLightsPSO;

	bool finalized{ false };
};