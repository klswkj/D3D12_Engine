#pragma once
#include "stdafx.h"

class IEntity;
class IWindow;
class MaterialWindow;

namespace custom
{
	class CommandContext;
	class GraphicsContext;
}

class RenderingResource
{
public:
	virtual ~RenderingResource() = default;
	virtual void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT = 0;
	virtual void InitializeParentReference(const IEntity&) noexcept {}
	virtual void RenderingWindow(const IWindow& _IWindow) {}
};

class CloningPtr : public RenderingResource
{
public:
	virtual std::unique_ptr<CloningPtr> Clone() const noexcept = 0;
};