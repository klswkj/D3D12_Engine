#pragma once
#include "RenderingResource.h"
// #include "IEntity.h"

class IEntity;

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
	void InitializeParentReference(const IEntity& _Entity) noexcept override;
	// void UpdateBind(custom::CommandContext& BaseContext);
	// Transform GetTransform(custom::CommandContext& BaseContext);

private:
	UINT m_RootIndex;
	const IEntity* m_pParent = nullptr;
	Transform m_Transform;
};