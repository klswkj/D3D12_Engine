#pragma once
#include "stdafx.h"
#include "Pass.h"

// Binding Pass + FullScreen Pass

class RenderingResource;
class RenderTarget;
class DepthStencil;
class ContainerBindableSink;

namespace custom
{
	class CommandContext;
}

// <- BindingPass
class BlurPass : public Pass
{
protected:
	BlurPass(std::string Name, std::vector<std::shared_ptr<RenderingResource>> binds = {});
	~BlurPass();
	void PushBackHorizontal(std::shared_ptr<RenderingResource> _RenderingResource) noexcept;
	void PushBackVertical(std::shared_ptr<RenderingResource> _RenderingResource) noexcept;
	void CalcGaussWeights(float Sigma, UINT Radius);

	void finalize() override;
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void RenderWindow();

	template<class T>
	void addBindSink(std::string name)
	{
		const size_t index = m_RenderingResourcesHorizontal.size();
		m_RenderingResourcesHorizontal.emplace_back();
		PushBackSink(std::make_unique<ContainerBindableSink<T>>(std::move(name), m_RenderingResourcesHorizontal, index));
	}

private:
	std::vector<std::shared_ptr<RenderingResource>> m_RenderingResourcesHorizontal;
	std::vector<std::shared_ptr<RenderingResource>> m_RenderingResourcesVertical;
	std::shared_ptr<RenderTarget> m_spRenderTarget;
	std::shared_ptr<DepthStencil> m_spDepthStencil;
	custom::RootSignature m_RootSignature1;
	GraphicsPSO m_GraphicsPSO;

	custom::RootSignature m_RootSignature2;
	ComputePSO m_HorizontalComputePSO;
	ComputePSO m_VerticalComputePSO;

	ColorBuffer m_PingPongBuffer1;
	ColorBuffer m_PingPongBuffer2;

	const UINT m_MaxRadius{ 5 }; // Keep Sync in HLSL.
	float m_BlurSigma{ 2.0f };
	int m_BlurRadius{ 3 };
	float m_Weights[11];
	int m_BlurCount{ 4 };
};