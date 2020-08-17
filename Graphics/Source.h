#pragma once
#include "stdafx.h"

class Bindable;
class IDisplaySurface;

class Source
{
public:
	const std::string& GetRegisteredName() const noexcept;
	virtual std::shared_ptr<Bindable> YieldBindable();
	virtual std::shared_ptr<IDisplaySurface> YieldBuffer();
	virtual ~Source() = default;
protected:
	Source(std::string name);
private:
	std::string name;
};

// BufferClearPass            <Bind::BufferResoruce>::Make("Buffer", buffer));
// LambertianPass             <RenderTarget>::Make("RenderTarget", renderTarget));
// LambertianPass             <DepthStencil>::Make("DepthStencil", depthStencil));
// OutlineDrawingPass         <RenderTarget>::Make("RenderTarget", rederTarget));
// OutlineDrawingPass         <DepthStencil>::Make("DepthStencil", depthStencil));
// OutlineMaskGenerationPass  <DepthStencil>::Make("DepthStencil", depthStencil));
// RenderGraph                <RenderTarget>::Make("Backbuffer", m_pBackBufferTarget));
// RenderGraph                <DepthStencil>::Make("MasterDepth", m_pMasterDepth));
// VerticalBlurPass           <RenderTarget>::Make("RenderTarget", renderTarget));
// VerticalBlurPass           <DepthStencil>::Make("DepthStencil", depthStencil));
// WireframePass              <RenderTarget>::Make("RenderTarget", renderTarget));
// WireframePass              <DepthStencil>::Make("Depthstencil", depthStencil));
template<class T>
class DirectBufferSource : public Source
{
public:
	static std::unique_ptr<DirectBufferSource> Make(std::string name, std::shared_ptr<T>& buffer)
	{
		return std::make_unique<DirectBufferSource>(std::move(name), buffer);
	}
	DirectBufferSource(std::string name, std::shared_ptr<T>& buffer)
		: Source(std::move(name)), buffer(buffer)
	{
	}
	std::shared_ptr<Bind::DisplaySurface> YieldBuffer() override
	{
		if (bLinked)
		{
			ASSERT(0, "Mutable output bound twice: ", GetRegisteredName());
		}
		bLinked = true;
		return buffer;
	}
private:
	std::shared_ptr<T>& buffer;
	bool bLinked = false;
};

// BlurOutlineDrawingPass  <Bind::RenderTarget>::Make("ScratchOut", renderTarget);
// BlurOutlineRenderGraph  <Bind::CachingPixelConstantBufferEx>::Make("BlurKenrel", blurKernel));
// BlurOutlineRenderGraph  <Bind::CachingPixelConstnatBufferEx>::Make("BlurDirection", blurDirection));
// HorizontalBlurPass      <Bind::RenderTarget>::Make("ScratchOut", renderTarget);
// ShadowMapplingPass      <Bind::DepthStencil>::Make("map, depthStencil));
template<class T>
class DirectBindableSource : public Source
{
public:
	static std::unique_ptr<DirectBindableSource> Make(std::string name, std::shared_ptr<T>& buffer)
	{
		return std::make_unique<DirectBindableSource>(std::move(name), buffer);
	}
	DirectBindableSource(std::string name, std::shared_ptr<T>& bind)
		: Source(std::move(name)), bind(bind)
	{
	}
	std::shared_ptr<Bindable> YieldBindable() override
	{
		return bind;
	}
private:
	std::shared_ptr<T>& bind;
};