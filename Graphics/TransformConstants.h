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
	__declspec(align(16)) struct Transform
	{
		Math::Matrix4 ModelMatrix;
	};

	TransformConstants(const UINT rootIndex);
	void Bind(custom::CommandContext& baseContext, const uint8_t commandIndex) final;
	void InitializeParentReference(const IEntity& _Entity) noexcept final;
	// void UpdateBind(custom::CommandContext& BaseContext);
	// Transform GetTransform(custom::CommandContext& BaseContext);

private:
	UINT m_RootIndex;
	const IEntity* m_pParent;
	Transform m_Transform;
};