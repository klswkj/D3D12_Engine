#pragma once
#include "RenderingResource.h"

namespace custom
{
	class CommandContext;
}

class IWindow;

class Color3Buffer : public RenderingResource
{
public:
	Color3Buffer(UINT RootIndex, DirectX::XMFLOAT3 InputColor = { 0.2f, 0.2f, 0.2f });
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	void SetColor(DirectX::XMFLOAT3 InputColor);
	void SetColorByComponent(float R, float G, float B);

	__declspec(align(16)) DirectX::XMFLOAT3 Color;
	UINT m_RootIndex;
};

class Color4Buffer : public RenderingResource
{
public:
	Color4Buffer(UINT RootIndex, DirectX::XMFLOAT4 InputColor = { 0.2f, 0.2f, 0.2f, 1.0f });
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	void SetColor(DirectX::XMFLOAT4 InputColor);
	void SetColorByComponent(float R, float G, float B, float A);

	__declspec(align(16)) DirectX::XMFLOAT4 Color;
	UINT m_RootIndex;
};
