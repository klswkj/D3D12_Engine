#include "stdafx.h"
#include "ColorConstants.h"
#include "CommandContext.h"

// #define FLOAT_COLOR3_ROOT_INDEX 3u
// #define FLOAT_COLOR4_ROOT_INDEX 3u

Color3Buffer::Color3Buffer(UINT RootIndex, DirectX::XMFLOAT3 InputColor)
{
	m_RootIndex = RootIndex;
	Color       = InputColor;
}
void Color3Buffer::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetDynamicConstantBufferView(m_RootIndex, sizeof(DirectX::XMFLOAT3), (void*)&Color);
}
void Color3Buffer::RenderingWindow(IWindow& _IWindow)
{
	ImGui::ColorPicker3("Color3", (float*)&Color);
}

void Color3Buffer::SetColor(DirectX::XMFLOAT3 InputColor)
{
	Color = InputColor;
}
void Color3Buffer::SetColorByComponent(float R, float G, float B)
{
	Color.x = R;
	Color.y = G;
	Color.z = B;
}

Color4Buffer::Color4Buffer(UINT RootIndex, DirectX::XMFLOAT4 InputColor)
{
	m_RootIndex = RootIndex;
	Color       = InputColor;
}
void Color4Buffer::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetDynamicConstantBufferView(m_RootIndex, sizeof(DirectX::XMFLOAT4), (void*)&Color);
}
void Color4Buffer::RenderingWindow(IWindow& _IWindow)
{
	ImGui::ColorPicker4("Color4", (float*)&Color);
}
void Color4Buffer::SetColor(DirectX::XMFLOAT4 InputColor)
{
	Color = InputColor;
}
void Color4Buffer::SetColorByComponent(float R, float G, float B, float A)
{
	Color.x = R;
	Color.y = G;
	Color.z = B;
	Color.w = A;
}