#include "stdafx.h"
#include "BlurPass.h"
#include "RenderingResource.h"
#include "VertexBuffer.h"
// Binding Pass + FullScreen Pass

BlurPass::BlurPass(std::string name, std::vector<std::shared_ptr<RenderingResource>> binds)
	: Pass(std::move(name)), binds(std::move(binds))
{

	// 여기서 PSO 조립.

	// FullScreenPass constructor
	// setup fullscreen geometry
	/*
	ShaderInputLayout lay;
	lay.Append(ShaderInputLayout::Position2D);
	VertexBuffer bufFull{ lay };
	bufFull.EmplaceBack(DirectX::XMFLOAT2{ -1,  1 });
	bufFull.EmplaceBack(DirectX::XMFLOAT2{  1,  1 });
	bufFull.EmplaceBack(DirectX::XMFLOAT2{ -1, -1 });
	bufFull.EmplaceBack(DirectX::XMFLOAT2{  1, -1 });
	insertBindable(Bind::VertexBuffer::Resolve("$Full", std::move(bufFull)));
	std::vector<unsigned short> indices = { 0,1,2,1,3,2 };
	insertBindable(Bind::IndexBuffer::Resolve(gfx, "$Full", std::move(indices)));
	// setup other common fullscreen bindables
	auto vs = Bind::VertexShader::Resolve(gfx, "Fullscreen_VS.cso");
	insertBindable(Bind::InputLayout::Resolve(gfx, lay, *vs));
	insertBindable(std::move(vs));
	insertBindable(Bind::Topology::Resolve(gfx));
	insertBindable(Bind::Rasterizer::Resolve(gfx, false));
	D3D12_INPUT_ELEMENT_DESC
	*/

	// Specific BlurPass Constructor
	/*
	insertBindable(PixelShader::Resolve(gfx, "BlurOutline_PS.cso"));
	insertBindable(Blender::Resolve(gfx, true));
	insertBindable(Stencil::Resolve(gfx, Stencil::Mode::Mask));
	insertBindable(Sampler::Resolve(gfx, Sampler::Type::Bilinear, true));

	addBindSink<RenderTarget>("scratchIn");
	addBindSink<CachingPixelConstantBufferEx>("kernel");
	PushBackSink(DirectBindableSink<CachingPixelConstantBufferEx>::Make("direction", direction));
	PushBackSink(DirectBufferSink<RenderTarget>::Make("renderTarget", renderTarget));
	PushBackSink(DirectBufferSink<DepthStencil>::Make("depthStencil", depthStencil));

	PushBackSource(DirectBufferSource<RenderTarget>::Make("renderTarget", renderTarget));
	PushBackSource(DirectBufferSource<DepthStencil>::Make("depthStencil", depthStencil));
	*/
}

void BlurPass::insertBindable(std::shared_ptr<RenderingResource> bind) noexcept
{
	binds.push_back(std::move(bind));
}

void BlurPass::bindAll() const noexcept
{
	bindBufferResources();
	for (auto& bind : binds)
	{
		bind->Bind(gfx);
	}
}

void BlurPass::finalize()
{
	Pass::finalize();
	if (!renderTarget && !depthStencil)
	{
		throw RGC_EXCEPTION("BindingPass [" + GetRegisteredName() + "] needs at least one of a renderTarget or depthStencil");
	}
}

void BlurPass::bindBufferResources() const DEBUG_EXCEPT
{
	if (renderTarget != nullptr)
	{
		renderTarget->BindAsBuffer(depthStencil.get());
	}
	else
	{
		depthStencil->BindAsBuffer();
	}
}

void BlurPass::Execute() const DEBUG_EXCEPT
{
	/* VerticalBlur
	auto buf = direction->GetBuffer();
		buf["isHorizontal"] = false;
		direction->SetBuffer(buf);
		direction->Bind(gfx);
	*/
	/* Horizontal
	auto buf = direction->GetBuffer();
		buf["isHorizontal"] = true;
		direction->SetBuffer(buf);
		direction->Bind(gfx);
	*/
	/*
	FullscreenPass::Execute(gfx);
	=>
	bindAll(gfx);
	gfx.DrawIndexed(6u);
	*/


	bindAll(/*gfx*/);
	// gfx.DrawIndexed(6u);
}