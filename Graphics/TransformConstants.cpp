#include "stdafx.h"
#include "TransformConstants.h"
#include "CommandContext.h"
#include "Entity.h"

TransformConstants::TransformConstants(UINT RootIndex)
{
	m_RootIndex = RootIndex;
	memset(&m_Transform, -1, sizeof(m_Transform));
}
void TransformConstants::Bind(custom::CommandContext& BaseContext)
{
	ASSERT(m_pParent != nullptr, "Didn't Initialize Parent Reference.");

	// m_Transform.ModelMatrix = m_pParent->GetTransform().GetTranspose();
	m_Transform.ModelMatrix = m_pParent->GetTransform();

	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetDynamicConstantBufferView(m_RootIndex, sizeof(m_Transform), &m_Transform);
}
void TransformConstants::InitializeParentReference(const Entity& _Entity) noexcept
{
	m_pParent = &_Entity;
}