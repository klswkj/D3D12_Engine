#pragma once
#include "Model.h"
#include "RootSignature.h"
#include "PSO.h"
#include "ShadowCamera.h"
#include "MainLight.h"

class RenderQueuePass;
class Camera;
class Pass;
class LightPrePass;
class ShadowPrePass;
class Z_PrePass;
class SSAOPass;
class FillLightGridPass;
class ShadowMappingPass;
class MainRenderPass;
enum class eObjectFilterFlag;

namespace custom
{
	class CommandContext;
}

namespace Math
{
	class Matrix4;
}

class LightPrePass;
class ShadowPrePass;
class Z_PrePass;
class SSAOPass;
class FillLightGridPass;
class ShadowMappingPass;
class MainRenderPass;

class MasterRenderGraph
{
public:
	MasterRenderGraph();
	~MasterRenderGraph();
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void Profiling();
	void Update();
	void Reset() noexcept;

	RenderQueuePass& FindRenderQueuePass(const std::string& RenderQueuePassName);
	Pass& FindPass(const std::string& PassName);

	void ShowPassesWindows();
	void BindMainCamera(Camera* pCamera);
	void BindMainLightContainer(std::vector<MainLight>* MainLightContainer);

private:
	void finalize();
	void appendPass(std::unique_ptr<Pass> pass);

public:
	std::vector<std::unique_ptr<Pass>> m_pPasses;
	Camera* m_pCurrentActiveCamera;

private:
	std::vector<MainLight>* m_pMainLights;

	LightPrePass* m_pLightPrePass;
	ShadowPrePass* m_pShadowPrePass;
	Z_PrePass* m_pZ_PrePass;
	SSAOPass* m_pSSAOPass;
	FillLightGridPass* m_pFillLightGridPass;
	ShadowMappingPass* m_pShadowMappingPass;
	MainRenderPass* m_pMainRenderPass;

	custom::RootSignature m_RootSignature;
	GraphicsPSO m_DepthPSO;
	GraphicsPSO m_ShadowPSO;
	GraphicsPSO m_MainRenderPSO;
};