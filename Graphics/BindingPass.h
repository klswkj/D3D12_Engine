#pragma once
#include "Pass.h"

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
	IBindingPass(const char* name, std::vector<std::shared_ptr<RenderingResource>> = {});
	void PushBack(std::shared_ptr<RenderingResource> spRenderingResource) noexcept;
	void bindAll(custom::CommandContext& BaseContext) const noexcept;
	void finalize() override;

	template<class T>
	void addBindSink(std::string name);

private:
	void bindBufferResources(custom::CommandContext&) const DEBUG_EXCEPT;

protected:
	std::shared_ptr<RenderTarget> m_pRenderTarget;
	std::shared_ptr<DepthStencil> m_pDepthStencil;

private:
	std::vector<std::shared_ptr<RenderingResource>> m_RenderingResources;
};