#pragma once
#include "RenderingResource.h"

namespace custom
{
	class CommandContext;
}

class IWindow;

class Color3Buffer final : public RenderingResource
{
public:
	Color3Buffer(const UINT RootIndex, const DirectX::XMFLOAT3 InputColor = { 0.2f, 0.2f, 0.2f });
	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	void SetColor(const DirectX::XMFLOAT3& InputColor);
	void SetColorByComponent(const float R, const float G, const float B);

	__declspec(align(16)) DirectX::XMFLOAT3 Color;
	UINT m_RootIndex;
};

class Color4Buffer final : public RenderingResource
{
public:
	Color4Buffer(const UINT RootIndex, const DirectX::XMFLOAT4 InputColor = { 0.2f, 0.2f, 0.2f, 1.0f });
	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	void SetColor(const DirectX::XMFLOAT4& InputColor);
	void SetColorByComponent(const float R, const float G, const float B, const float A);

	__declspec(align(16)) DirectX::XMFLOAT4 Color;
	UINT m_RootIndex;
};
