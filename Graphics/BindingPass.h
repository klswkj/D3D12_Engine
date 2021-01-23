#pragma once
#include "Pass.h"

// Not Used.

namespace custom
{
	class CommandContext;
}

class RenderingResource;
class RenderTarget;
class DepthStencil;

class IBindingPass : public Pass
{
public:
protected:
	IBindingPass(std::string name, std::vector<std::shared_ptr<RenderingResource>> = {});
	void PushBack(std::shared_ptr<RenderingResource> spRenderingResource) noexcept;
	void bindAll(custom::CommandContext& BaseContext) const noexcept;
	void finalize() override;

private:
	void bindBufferResources(custom::CommandContext&) const DEBUG_EXCEPT;

protected:
	std::shared_ptr<RenderTarget> m_pRenderTarget = nullptr;
	std::shared_ptr<DepthStencil> m_pDepthStencil = nullptr;

private:
	std::vector<std::shared_ptr<RenderingResource>> m_RenderingResources;
};