#include "stdafx.h"
#include "TransformConstants.h"
#include "CommandContext.h"
#include "Entity.h"

TransformConstants::TransformConstants(const UINT rootIndex)
	:
	m_pParent(nullptr)
{
	m_RootIndex = rootIndex;
	memset(&m_Transform, -1, sizeof(m_Transform));
}
void TransformConstants::Bind(custom::CommandContext& baseContext, const uint8_t commandIndex)
{
	ASSERT(m_pParent != nullptr, "Didn't Initialize Parent Reference.");

	// m_Transform.ModelMatrix = m_pParent->GetTransform().GetTranspose();
	m_Transform.ModelMatrix = m_pParent->GetTransform();

	baseContext.GetGraphicsContext().SetDynamicConstantBufferView(m_RootIndex, sizeof(m_Transform), &m_Transform, commandIndex);
}
void TransformConstants::InitializeParentReference(const IEntity& _Entity) noexcept
{
	m_pParent = &_Entity;
}