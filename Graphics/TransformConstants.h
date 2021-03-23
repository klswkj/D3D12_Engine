#pragma once
#include "RenderingResource.h"
// #include "IEntity.h"

class IEntity;

namespace custom
{
	class CommandContext;
	class GraphicsContext;
}

class TransformConstants final : public RenderingResource
{
public:
	TransformConstants(const UINT rootIndex);
	void Bind(custom::CommandContext& baseContext, const uint8_t commandIndex) final;
	void InitializeParentReference(const IEntity& _Entity) noexcept final;
	// void UpdateBind(custom::CommandContext& BaseContext);
	// Transform GetTransform(custom::CommandContext& BaseContext);

private:
	UINT m_RootIndex;
	const IEntity* m_pParent;
	Math::Matrix4 m_Transform;
};