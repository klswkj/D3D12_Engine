#pragma once
#include "stdafx.h"
#include "Pass.h"
#include "RootSignature.h"

// Binding Pass + FullScreen Pass

class RenderingResource;
class RenderTarget;
class DepthStencil;

class BlurPass : public Pass
{
protected:
	BlurPass(std::string name, std::vector<std::shared_ptr<RenderingResource>> binds = {});
	void insertBindable(std::shared_ptr<RenderingResource> bind) noexcept;
	void bindAll(/*Graphics& gfx*/) const noexcept;
	void finalize() override;
	void Execute(/*Graphics& gfx*/) const DEBUG_EXCEPT override;
	template<class T>
	void addBindSink(std::string name)
	{
		const auto index = binds.size();
		binds.emplace_back();
		PushBackSink(std::make_unique<ContainerBindableSink<T>>(std::move(name), binds, index));
	}
protected:
	///////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////
	// D3D11 Version

	std::vector<std::shared_ptr<RenderingResource>> binds;
	std::shared_ptr<RenderTarget> renderTarget;
	std::shared_ptr<DepthStencil> depthStencil;

	//////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////
	// D3D12 Version

	custom::RootSignature m_RootSignature;
};