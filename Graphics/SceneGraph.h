#pragma once
#include "stdafx.h"

// #include "UpdateConstantBuffer.h"

class Graphics; // #define "Graphics.h"
class Camera; // #define "Camera.h"

namespace custom
{
	class RenderTarget;
	class DepthStencil;
}

class Pass;
class RenderQueuePass;
class Source;
class Sink;

class SceneGraph // MasterRenderGraph
{
public:
	SceneGraph();
	~SceneGraph();
	void Execute(Graphics& gfx) DEBUG_EXCEPT;
	void Reset() noexcept;
	RenderQueuePass& GetRenderQueue(const std::string& passName);

	// BlurOutlineRenderGraph(Graphics& gfx);
	void RenderWidgets(Graphics& gfx);
	void BindMainCamera(Camera& cam);
	void BindShadowCamera(Camera& cam);

private:
	void setSinkTarget(const std::string& sinkName, const std::string& target);
	void setKernelGauss(int radius, float sigma) DEBUG_EXCEPT;
	void setKernelBox(int radius) DEBUG_EXCEPT;

	void pushbackSource(std::unique_ptr<Source>);
	void pushbackSink(std::unique_ptr<Sink>);
	void appendPass(std::unique_ptr<Pass> pass);
	void linkSinks(Pass& pass);
	Pass& findPassByName(const std::string& name);
	void finalize();

private:
	std::vector<std::unique_ptr<Pass>> m_pPasses;
	std::vector<std::unique_ptr<Source>> m_pGlobalSources;
	std::vector<std::unique_ptr<Sink>> m_pGlobalSinks;
	std::shared_ptr<Bind::RenderTarget> m_pBackBufferTarget;
	std::shared_ptr<Bind::DepthStencil> m_pMasterDepth;
	std::shared_ptr<Bind::CachingPixelConstantBufferEx> blurKernel;      // Imgui coefficient
	std::shared_ptr<Bind::CachingPixelConstantBufferEx> blurDirection;   // Imgui coefficient

	bool finalized = false;

	enum class KernelType
	{
		Gauss,
		Box,
	} kernelType = KernelType::Gauss; // Gauss, Box

	static constexpr int maxRadius = 7;
	int radius = 4;
	float sigma = 2.0f;
};
