#include "stdafx.h"
#include "ColorConstants.h"
#include "CommandContext.h"

// #define FLOAT_COLOR3_ROOT_INDEX 3u
// #define FLOAT_COLOR4_ROOT_INDEX 3u

Color3Buffer::Color3Buffer(const UINT RootIndex, const DirectX::XMFLOAT3 InputColor)
{
	m_RootIndex = RootIndex;
	Color       = InputColor;
}
void Color3Buffer::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetDynamicConstantBufferView(m_RootIndex, sizeof(DirectX::XMFLOAT3), (void*)&Color, commandIndex);
}
void Color3Buffer::RenderingWindow(const IWindow& _IWindow)
{
	ImGui::ColorPicker3("Color3", (float*)&Color);
}

void Color3Buffer::SetColor(const DirectX::XMFLOAT3& InputColor)
{
	Color = InputColor;
}
void Color3Buffer::SetColorByComponent(const float R, const float G, const float B)
{
	Color.x = R;
	Color.y = G;
	Color.z = B;
}

Color4Buffer::Color4Buffer(const UINT RootIndex, const DirectX::XMFLOAT4 InputColor)
{
	m_RootIndex = RootIndex;
	Color       = InputColor;
}
void Color4Buffer::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetDynamicConstantBufferView(m_RootIndex, sizeof(DirectX::XMFLOAT4), (void*)&Color, commandIndex);
}
void Color4Buffer::RenderingWindow(const IWindow& _IWindow)
{
	ImGui::ColorPicker4("Color4", (float*)&Color);
}
void Color4Buffer::SetColor(const DirectX::XMFLOAT4& InputColor)
{
	Color = InputColor;
}
void Color4Buffer::SetColorByComponent(const float R, const float G, const float B, const float A)
{
	Color.x = R;
	Color.y = G;
	Color.z = B;
	Color.w = A;
}