#pragma once
#include "Model.h"
#include "RootSignature.h"
#include "PSO.h"
#include "ShadowCamera.h"
#include "MainLight.h"

#pragma region FORWARD_DECLARATION
namespace custom
{
	class CommandContext;
}
namespace Math
{
	class Matrix4;
}

class ID3D12RenderQueuePass;
class Camera;
class D3D12Pass;
class LightPrePass;
class ShadowPrePass;
class Z_PrePass;
class SSAOPass;
class FillLightGridPass;
class ShadowMappingPass;
class MainRenderPass;
class OutlineDrawingPass;
class DebugWireFramePass;
class DebugShadowMapPass;
enum class eObjectFilterFlag;
#pragma endregion FORWARD_DECLARATION

// 시작할 때,
// TaskFiber* taskFiber = GPUWorkManager::RequestWorkSubmission(numPairALs, type, CompletedFenceValue);
// -> 그럴려면 일단 CommandContext에 QAL이 비어있어야하고,
// -> 내가 QAL을 끼우고 싶을 때 끼워야함.
// CommandContext::begin("", false)하면 QAL비게.

class MasterRenderGraph
{
	friend class D3D12Engine;
public:
	MasterRenderGraph();
	~MasterRenderGraph();
	void Execute() DEBUG_EXCEPT;
	void ExecuteWithMultiThread();
	void Profiling();
	void Update();
	void Reset() noexcept;

	ID3D12RenderQueuePass* FindRenderQueuePass(const std::string& RenderQueuePassName) const;
	D3D12Pass& FindPass(const std::string& PassName);

	void ShowPassesWindows();
	void BindMainCamera(Camera* pCamera);
	void BindMainLightContainer(std::vector<MainLight>* MainLightContainer);

	void RenderPassesWindow();

	void ToggleFullScreenDebugPasses();
	void ToggleDebugPasses();
	void ToggleDebugShadowMap();
	void ToggleSSAOPass();
	void ToggleSSAODebugMode();

private:
	void finalize();
	void appendPass               (std::unique_ptr<D3D12Pass> _Pass);
	void appendFullScreenDebugPass(std::unique_ptr<D3D12Pass> _Pass);
	void appendDebugPass          (std::unique_ptr<D3D12Pass> _Pass);

	void enableShadowMappingPass();
	void disableShadowMappingPass();
	void enableMainRenderPass();
	void disableMainRenderPass();
	void enableGPUTaskFiberDirty();

public:
	static MasterRenderGraph* s_pMasterRenderGraph;

	std::vector<std::unique_ptr<D3D12Pass>> m_pPasses;
	std::vector<std::unique_ptr<D3D12Pass>> m_pFullScreenDebugPasses;
	std::vector<std::unique_ptr<D3D12Pass>> m_pDebugPasses;
	std::vector<MainLight>*                 m_pMainLights;
	Camera* m_pCurrentActiveCamera;
	
private:
	LightPrePass*       m_pLightPrePass;
	ShadowPrePass*      m_pShadowPrePass;
	Z_PrePass*          m_pZ_PrePass;
	SSAOPass*           m_pSSAOPass;
	FillLightGridPass*  m_pFillLightGridPass;
	ShadowMappingPass*  m_pShadowMappingPass;
	MainRenderPass*     m_pMainRenderPass;
	OutlineDrawingPass* m_pOutlineDrawingPass;
	DebugWireFramePass* m_pDebugWireFramePass;
	DebugShadowMapPass* m_pDebugShadowMapPass;

	custom::RootSignature m_RootSignature;
	GraphicsPSO m_DepthPSO;
	GraphicsPSO m_ShadowPSO;
	GraphicsPSO m_MainRenderPSO;

	bool m_bFullScreenDebugPasses;
	bool m_bDebugPassesMode;
	bool m_bDebugShadowMap;    //
	bool m_bSSAOPassEnable;    //
	bool m_bSSAODebugDraw;     //
	bool m_bShadowMappingPass; //
	bool m_bMainRenderPass;    //
	bool m_bGPUTaskFiberDirty;

	// Bundle은 여러 쓰레드에서 공통적으로 쓸 수 있을 때만 생성하고,
	// 그리고 RS, PSO, DescriptorHeap, 뺴고 기타 상태만 바꿀때만
	UINT   m_NumNeedCommandQueues;
	// UINT m_NumNeedDirectCommandQueues  = 1U;
	// UINT m_NumNeedComputeCommandQueues = 1U;
	// UINT m_NumNeedCopyCommandQueues    = 1U;
	UINT   m_NumNeedCommandALs;
	size_t m_SelctedPassIndex;
};

#pragma region TOGGLE

inline void MasterRenderGraph::ToggleFullScreenDebugPasses()
{
	m_bFullScreenDebugPasses = !m_bFullScreenDebugPasses;
}
inline void MasterRenderGraph::ToggleDebugPasses()
{
	m_bDebugPassesMode = !m_bDebugPassesMode;
}
inline void MasterRenderGraph::ToggleDebugShadowMap()
{
	m_bDebugShadowMap = !m_bDebugShadowMap;
}
inline void MasterRenderGraph::ToggleSSAOPass()
{
	m_bSSAOPassEnable = !m_bSSAOPassEnable;
}

inline void MasterRenderGraph::enableShadowMappingPass()
{
	m_bShadowMappingPass = true;
}
inline void MasterRenderGraph::disableShadowMappingPass()
{
	m_bShadowMappingPass = false;
}

inline void MasterRenderGraph::enableMainRenderPass()
{
	m_bMainRenderPass = true;
}
inline void MasterRenderGraph::disableMainRenderPass()
{
	m_bMainRenderPass = false;
}

inline void MasterRenderGraph::enableGPUTaskFiberDirty()
{
	m_bGPUTaskFiberDirty = true;
}

#pragma endregion TOGGLE