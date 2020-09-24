#pragma once
#include "Model.h"
#include "RootSignature.h"
#include "PSO.h"
#include "ShadowCamera.h"

#include "Entity.h"
#include "Model.h"

class RenderQueuePass;
class Camera;
class Source;
class Sink;
class Pass;
class RenderTarget;
class DepthStencil;
enum class eObjectFilterFlag;

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

	RenderQueuePass& FindRenderQueuePass(const std::string& RenderQueuePassName);
	Pass& FindPass(const std::string& PassName);

	void ShowPassesWindows();
	void BindMainCamera(Camera* pCamera);
	void BindMainLightContainer(std::vector<Math::Matrix4>* MainLightContainer);
	void BindShadowMatrix(Math::Matrix4& _ShadowMatrix);

private:
	void finalize();
	void appendPass(std::unique_ptr<Pass> pass);

public:
	std::vector<std::unique_ptr<Pass>> m_pPasses;
	Camera* m_pCurrentActiveCamera{ nullptr };

private:
	std::vector<Math::Matrix4>* m_pSunShadows;

	// 그냥 컨테이너 GetShadowMatrix(), ShadowUpdate() 싸개
	// MasterRenderGraph에서의 ShadowCamera의 의의?
	// 
	ShadowCamera m_SunShadowCamera; 

	custom::RootSignature m_RootSignature;
	GraphicsPSO m_DepthPSO;
	GraphicsPSO m_ShadowPSO;
	GraphicsPSO m_MainRenderPSO;

	bool finalized{ false };
};