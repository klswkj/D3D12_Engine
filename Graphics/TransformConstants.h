#pragma once
#include "RenderingResource.h"
// #include "Entity.h"

class Entity;

namespace custom
{
	class CommandContext;
}

class TransformConstants : public RenderingResource
{
public:
	__declspec(align(16)) struct Transform
	{
		Math::Matrix4 ModelMatrix;
	};

	TransformConstants(UINT RootIndex);
	void Bind(custom::CommandContext& BaseContext) override;
	void InitializeParentReference(const Entity& _Entity) noexcept override;
	// void UpdateBind(custom::CommandContext& BaseContext);
	// Transform GetTransform(custom::CommandContext& BaseContext);

private:
	UINT m_RootIndex;
	const Entity* m_pParent = nullptr;
	Transform m_Transform;
};