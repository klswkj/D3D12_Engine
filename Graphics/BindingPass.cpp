#include "stdafx.h"
#include "BindingPass.h"
#include "Pass.h"
#include "RenderTarget.h"
#include "DepthStencil.h"

class ContainerBindableSink;

IBindingPass::IBindingPass(std::string name, std::vector<std::shared_ptr<RenderingResource>> otherContainer /*= {}*/)
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
	// bindBufferResources(BaseContext);

	for (auto& RenderingResource : m_RenderingResources)
	{
		RenderingResource->Bind(BaseContext);
	}
}

void IBindingPass::finalize()
{
	Pass::finalize();

	// ASSERT(m_pRenderTarget != nullptr, "BindingPass ", GetRegisteredName(), "RenderTarget don't exist.");
	// ASSERT(m_pDepthStencil != nullptr, "BindingPass ", GetRegisteredName(), "DepthStencil don't exist.");
}