#include "BindingPass.h"
#include "Pass.h"
#include "RenderTarget.h"
#include "DepthStencil.h"

IBindingPass::IBindingPass(const char* name, std::vector<std::shared_ptr<RenderingResource>> otherContainer /*= {}*/)
	: Pass(name), m_RenderingResources(std::move(otherContainer)) {}

void IBindingPass::PushBack(std::shared_ptr<RenderingResource> spRenderingResource) noexcept
{
	m_RenderingResources.push_back(std::move(spRenderingResource));
}

void IBindingPass::bindBufferResources(custom::CommandContext& BaseContext) const DEBUG_EXCEPT 
{
	if (m_pRenderTarget != nullptr)
	{
		m_pRenderTarget->BindAsBuffer(BaseContext, m_pDepthStencil.get());
	}
	else if (m_pDepthStencil != nullptr)
	{
		m_pDepthStencil->BindAsBuffer(BaseContext);
	}
	else
	{
		ASSERT(false, "BindingPass ", GetRegisteredName(), " : RenderTarget and DepthStencil are existed.");
	}
}

void IBindingPass::bindAll(custom::CommandContext& BaseContext) const noexcept
{
	bindBufferResources(BaseContext);

	for (auto& RenderingResource : m_RenderingResources)
	{
		RenderingResource->Bind(BaseContext);
	}
}

void IBindingPass::finalize()
{
	Pass::finalize();

	if (m_pRenderTarget == nullptr && m_pDepthStencil == nullptr)
	{
		ASSERT(false, "BindingPass ", GetRegisteredName(), " : RenderTarget and DepthStencil don't exist.");
	}
}

template<class T>
void IBindingPass::addBindSink(std::string name)
{
	const auto index = m_RenderingResources.size();
	m_RenderingResources.emplace_back();
	PushBackSink(std::make_unique<ContainerBindableSink<T>>(std::move(name), m_RenderingResources, index));
}
