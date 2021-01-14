#include "stdafx.h"
#include "Mesh.h"
#include "FundamentalVertexIndex.h"

Mesh::Mesh(const Material& CMaterial, FundamentalVertexIndex& Input, const float* pStartVertexLocation)
	: Entity(CMaterial, Input, pStartVertexLocation)
{
	ZeroMemory(&m_Transform, sizeof(m_Transform));
}

void Mesh::Submit(eObjectFilterFlag _Filter, Math::Matrix4 _AccumulatedTranform) const DEBUG_EXCEPT
{
	// DirectX::XMStoreFloat4x4(&m_Transform, _AccumulatedTranform);
	m_Transform = _AccumulatedTranform;
	Entity::Submit(_Filter);
}

Math::Matrix4 Mesh::GetTransform() const noexcept
{
	// return DirectX::XMLoadFloat4x4(&m_Transform);
	return m_Transform;
}
