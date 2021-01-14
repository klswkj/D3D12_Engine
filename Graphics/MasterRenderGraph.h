#pragma once
#include "Model.h"
#include "RootSignature.h"
#include "PSO.h"
#include "ShadowCamera.h"
#include "MainLight.h"

namespace custom
{
	class CommandContext;
}
namespace Math
{
	class Matrix4;
}
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
class DebugWireFramePass;
class DebugShadowMapPass;
enum class eObjectFilterFlag;

class MasterRenderGraph
{
	friend class D3D12Engine;
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

	void ToggleFullScreenDebugPasses();
	void ToggleDebugPasses();
	void ToggleDebugShadowMap();
	void ToggleSSAOPass();
	void ToggleSSAODebugMode();
private:
	void finalize();
	void appendPass               (std::unique_ptr<Pass> _Pass);
	void appendFullScreenDebugPass(std::unique_ptr<Pass> _Pass);
	void appendDebugPass          (std::unique_ptr<Pass> _Pass);

	void enableShadowMappingPass();
	void disableShadowMappingPass();
	void enableMainRenderPass();
	void disableMainRenderPass();
public:
	static MasterRenderGraph* s_pMasterRenderGraph;
	std::vector<std::unique_ptr<Pass>> m_pPasses;
	std::vector<std::unique_ptr<Pass>> m_pFullScreenDebugPasses;
	std::vector<std::unique_ptr<Pass>> m_pDebugPasses;
	std::vector<MainLight>* m_pMainLights;
	Camera* m_pCurrentActiveCamera;
	
	bool m_bFullScreenDebugPasses;
	bool m_bDebugPasses;
	bool m_bDebugShadowMap;    //
	bool m_bSSAOPassEnable;    //
	bool m_bSSAODebugDraw;     //
	bool m_bShadowMappingPass; //
	bool m_bMainRenderPass;    //
private:
	LightPrePass*       m_pLightPrePass;
	ShadowPrePass*      m_pShadowPrePass;
	Z_PrePass*          m_pZ_PrePass;
	SSAOPass*           m_pSSAOPass;
	FillLightGridPass*  m_pFillLightGridPass;
	ShadowMappingPass*  m_pShadowMappingPass;
	MainRenderPass*     m_pMainRenderPass;
	DebugWireFramePass* m_pDebugWireFramePass;
	DebugShadowMapPass* m_pDebugShadowMapPass;

	custom::RootSignature m_RootSignature;
	GraphicsPSO m_DepthPSO;
	GraphicsPSO m_ShadowPSO;
	GraphicsPSO m_MainRenderPSO;
};