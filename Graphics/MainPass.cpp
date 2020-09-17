#include "stdafx.h"
#include "MainPass.h"
#include "CommandContext.h"
#include "ShadowCamera.h"

MainPass::MainPass(const char* pName)
	: Pass(pName) {}

void MainPass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(0, sizeof(graphicsContext.m_VSConstants), &graphicsContext.m_VSConstants);

}
void MainPass::Reset() DEBUG_EXCEPT
{

}