#include "stdafx.h"
#include "SceneGraph.h"

#include "BindableCommon.h"
#include "DynamicConstant.h"
#include "RenderTarget.h"
#include "DepthStencil.h"
#include "Camera.h"

#include "Sink.h"
#include "Source.h"
#include "Pass.h"
/*
#include "HorizontalBlurPass.h"
#include "VerticalBlurPass.h"
#include "BlurOutlineDrawingPass.h"
#include "WireframePass.h"
#include "ShadowMappingPass.h"
#include "BufferClearPass.h"
#include "LambertianPass.h"
#include "OutlineDrawingPass.h"
#include "OutlineMaskGenerationPass.h"
*/
#include "imgui.h"

SceneGraph::SceneGraph()
	: m_pBackBufferTarget(/*Render Target*/), m_pMasterDepth(std::make_shared<Bind::OutputOnlyDepthStencil>(gfx))
{
	{
		// setup global sinks and sources
		pushbackSource(DirectBufferSource<Bind::RenderTarget>::Make("backbuffer", m_pBackBufferTarget));
		pushbackSource(DirectBufferSource<Bind::DepthStencil>::Make("masterDepth", m_pMasterDepth));
		pushbackSink(DirectBufferSink<Bind::RenderTarget>::Make("backbuffer", m_pBackBufferTarget));
	}
	{
		auto pass = std::make_unique<BufferClearPass>("clearRT");
		pass->SetSinkLinkage("buffer", "$.backbuffer");
		appendPass(std::move(pass));
	}
	{
		auto pass = std::make_unique<BufferClearPass>("clearDS");
		pass->SetSinkLinkage("buffer", "$.masterDepth");
		appendPass(std::move(pass));
	}
	{
		auto pass = std::make_unique<ShadowMappingPass>(gfx, "shadowMap");
		appendPass(std::move(pass));
	}
	{
		auto pass = std::make_unique<LambertianPass>(gfx, "lambertian");
		pass->SetSinkLinkage("shadowMap", "shadowMap.map");
		pass->SetSinkLinkage("renderTarget", "clearRT.buffer");
		pass->SetSinkLinkage("depthStencil", "clearDS.buffer");
		appendPass(std::move(pass));
	}
	{
		auto pass = std::make_unique<OutlineMaskGenerationPass>(gfx, "outlineMask");
		pass->SetSinkLinkage("depthStencil", "lambertian.depthStencil");
		appendPass(std::move(pass));
	}

	// setup blur constant buffers
	{
		{
			Dcb::RawLayout Coeff;
			Coeff.Add<Dcb::Integer>("nTaps");
			Coeff.Add<Dcb::Array>("coefficients");
			Coeff["coefficients"].Set<Dcb::Float>(maxRadius * 2 + 1);
			Dcb::Buffer buf{ std::move(Coeff) };
			blurKernel = std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, buf, 0);
			setKernelGauss(radius, sigma);
			pushbackSource(DirectBindableSource<Bind::CachingPixelConstantBufferEx>::Make("blurKernel", blurKernel));
		}
		{
			Dcb::RawLayout bHorizontal;
			bHorizontal.Add<Dcb::Bool>("isHorizontal");
			Dcb::Buffer buf{ std::move(bHorizontal) };
			blurDirection = std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, buf, 1);
			pushbackSource(DirectBindableSource<Bind::CachingPixelConstantBufferEx>::Make("blurDirection", blurDirection));
		}
	}

	{
		auto pass = std::make_unique<BlurOutlineDrawingPass>(gfx, "outlineDraw", gfx.GetWidth(), gfx.GetHeight());
		appendPass(std::move(pass));
	}
	{
		auto pass = std::make_unique<HorizontalBlurPass>("horizontal", gfx, gfx.GetWidth(), gfx.GetHeight());
		pass->SetSinkLinkage("scratchIn", "outlineDraw.scratchOut");
		pass->SetSinkLinkage("kernel", "$.blurKernel");
		pass->SetSinkLinkage("direction", "$.blurDirection");
		appendPass(std::move(pass));
	}
	{
		auto pass = std::make_unique<VerticalBlurPass>("vertical", gfx);
		pass->SetSinkLinkage("renderTarget", "lambertian.renderTarget");
		pass->SetSinkLinkage("depthStencil", "outlineMask.depthStencil");
		pass->SetSinkLinkage("scratchIn", "horizontal.scratchOut");
		pass->SetSinkLinkage("kernel", "$.blurKernel");
		pass->SetSinkLinkage("direction", "$.blurDirection");
		appendPass(std::move(pass));
	}
	{
		auto pass = std::make_unique<WireframePass>(gfx, "wireframe");
		pass->SetSinkLinkage("renderTarget", "vertical.renderTarget");
		pass->SetSinkLinkage("depthStencil", "vertical.depthStencil");
		appendPass(std::move(pass));
	}
	setSinkTarget("backbuffer", "wireframe.renderTarget");

	finalize();
}

SceneGraph::~SceneGraph()
{
}

void SceneGraph::Execute(Graphics& gfx) DEBUG_EXCEPT
{
	assert(finalized);
	for (auto& p : m_pPasses)
	{
		p->Execute(gfx);
	}
}

void SceneGraph::Reset() noexcept
{
	assert(finalized);
	for (auto& p : m_pPasses)
	{
		p->Reset();
	}
}

RenderQueuePass& SceneGraph::GetRenderQueue(const std::string& passName)
{
	try
	{
		for (const auto& p : m_pPasses)
		{
			if (p->GetRegisteredName() == passName)
			{
				return dynamic_cast<RenderQueuePass&>(*p);
			}
		}
	}
	catch (std::bad_cast&)
	{
		ASSERT(0, "Bad_Cast");
	}

	ASSERT(0, "In MasterRenderGraph::GetRenderQueue, pass cannot be found : ", passName);
}

void SceneGraph::RenderWidgets(/*Graphics& gfx*/)
{
	if (ImGui::Begin("Kernel"))
	{
		bool filterChanged = false;
		{
			const char* items[] = { "Gauss", "Box" };
			static const char* curItem = items[0];
			if (ImGui::BeginCombo("Filter Type", curItem))
			{
				for (int n = 0; n < std::size(items); ++n)
				{
					const bool isSelected = (curItem == items[n]);
					if (ImGui::Selectable(items[n], isSelected))
					{
						filterChanged = true;
						curItem = items[n];
						if (curItem == items[0])
						{
							kernelType = KernelType::Gauss;
						}
						else if (curItem == items[1])
						{
							kernelType = KernelType::Box;
						}
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		bool radChange = ImGui::SliderInt("Radius", &radius, 0, maxRadius);
		bool sigChange = ImGui::SliderFloat("Sigma", &sigma, 0.1f, 10.0f);
		if (radChange || sigChange || filterChanged)
		{
			if (kernelType == KernelType::Gauss)
			{
				setKernelGauss(radius, sigma);
			}
			else if (kernelType == KernelType::Box)
			{
				setKernelBox(radius);
			}
		}
	}
	ImGui::End();
}

void SceneGraph::BindMainCamera(Camera& cam)
{
	dynamic_cast<LambertianPass&>(findPassByName("lambertian")).BindMainCamera(cam);
}

void SceneGraph::BindShadowCamera(Camera& cam)
{
	dynamic_cast<ShadowMappingPass&>(findPassByName("shadowMap")).BindShadowCamera(cam);
	dynamic_cast<LambertianPass&>(findPassByName("lambertian")).BindShadowCamera(cam);
}

void SceneGraph::finalize()
{
	assert(!finalized);
	for (const auto& p : m_pPasses)
	{
		p->finalize();
	}

	// Do Link Global Sinks.
	for (auto& sink : m_pGlobalSinks)
	{
		const auto& inputSourcePassName = sink->GetPassName();
		for (auto& existingPass : m_pPasses)
		{
			if (existingPass->GetRegisteredName() == inputSourcePassName)
			{
				auto& source = existingPass->GetSource(sink->GetOutputName());
				sink->Bind(source);
				break;
			}
		}
	}

	finalized = true;
}

void SceneGraph::setSinkTarget(const std::string& sinkName, const std::string& target)
{
	const auto finder = [&sinkName](const std::unique_ptr<Sink>& p)
	{
		return p->GetRegisteredName() == sinkName;
	};
	const auto i = std::find_if(m_pGlobalSinks.begin(), m_pGlobalSinks.end(), finder);
	if (i == m_pGlobalSinks.end())
	{
		throw RGC_EXCEPTION("Global sink does not exist: " + sinkName);
	}

	auto targetSplit = SplitString(target, ".");
	if (targetSplit.size() != 2u)
	{
		throw RGC_EXCEPTION("Input target has incorrect format");
	}
	(*i)->SetTarget(targetSplit[0], targetSplit[1]);
}

void SceneGraph::setKernelGauss(int radius, float sigma) DEBUG_EXCEPT
{
	assert(radius <= maxRadius);
	auto k = blurKernel->GetBuffer();
	const int nTaps = radius * 2 + 1;
	k["nTaps"] = nTaps;
	float sum = 0.0f;
	for (int i = 0; i < nTaps; ++i)
	{
		const auto x = float(i - radius);
		const auto g = gauss(x, sigma);
		sum += g;
		k["coefficients"][i] = g;
	}
	for (int i = 0; i < nTaps; i++)
	{
		k["coefficients"][i] = (float)k["coefficients"][i] / sum;
	}
	blurKernel->SetBuffer(k);
}

void SceneGraph::setKernelBox(int radius) DEBUG_EXCEPT
{
	assert(radius <= maxRadius);
	auto k = blurKernel->GetBuffer();
	const int nTaps = radius * 2 + 1;
	k["nTaps"] = nTaps;
	const float c = 1.0f / nTaps;
	for (int i = 0; i < nTaps; i++)
	{
		k["coefficients"][i] = c;
	}
	blurKernel->SetBuffer(k);
}

void SceneGraph::linkSinks(Pass& pass)
{
	for (auto& si : pass.GetSinks())
	{
		const auto& inputSourcePassName = si->GetPassName();

		if (inputSourcePassName.empty())
		{
			std::ostringstream oss;
			oss << "In pass named [" << pass.GetRegisteredName() << "] sink named [" << si->GetRegisteredName() << "] has no target source set.";
			throw RGC_EXCEPTION(oss.str());
		}

		// check check whether target source is global
		if (inputSourcePassName == "$")
		{
			bool bound = false;
			for (auto& source : m_pGlobalSources)
			{
				if (source->GetRegisteredName() == si->GetOutputName())
				{
					si->Bind(*source);
					bound = true;
					break;
				}
			}
			if (!bound)
			{
				std::ostringstream oss;
				oss << "Output named [" << si->GetOutputName() << "] not found in globals";
				throw RGC_EXCEPTION(oss.str());
			}
		}
		else // find source from within existing passes
		{
			bool bound = false;
			for (auto& existingPass : m_pPasses)
			{
				if (existingPass->GetRegisteredName() == inputSourcePassName)
				{
					auto& source = existingPass->GetSource(si->GetOutputName());
					si->Bind(source);
					bound = true;
					break;
				}
			}
			if (!bound)
			{
				std::ostringstream oss;
				oss << "Pass named [" << inputSourcePassName << "] not found";
				throw RGC_EXCEPTION(oss.str());
			}
		}
	}
}

void SceneGraph::pushbackSource(std::unique_ptr<Source> out)
{
	m_pGlobalSources.push_back(std::move(out));
}

void SceneGraph::pushbackSink(std::unique_ptr<Sink> in)
{
	m_pGlobalSinks.push_back(std::move(in));
}

void SceneGraph::appendPass(std::unique_ptr<Pass> pass)
{
	assert(!finalized);
	// validate name uniqueness
	for (const auto& p : m_pPasses)
	{
		if (pass->GetRegisteredName() == p->GetRegisteredName())
		{
			throw RGC_EXCEPTION("Pass name already exists: " + pass->GetRegisteredName());
		}
	}

	// link outputs from passes (and global outputs) to pass inputs
	linkSinks(*pass);

	// add to container of passes
	m_pPasses.push_back(std::move(pass));
}

Pass& SceneGraph::findPassByName(const std::string& name)
{
	const auto i = std::find_if(m_pPasses.begin(), m_pPasses.end(), [&name](auto& p) {
		return p->GetRegisteredName() == name;
		});
	if (i == m_pPasses.end())
	{
		throw std::runtime_error{ "Failed to find pass name" };
	}
	return **i;
}