#include "stdafx.h"
#include "Mesh.h"
#include "ObjectFilterFlag.h"

Mesh::Mesh(const Material& _Material, const aiMesh& mesh, float scale) DEBUG_EXCEPT
	: Entity(_Material, mesh, scale)
{
	ZeroMemory(&m_Transform, sizeof(m_Transform));
}

void Mesh::Submit(eObjectFilterFlag _Filter, DirectX::FXMMATRIX _AccumulatedTranform) const DEBUG_EXCEPT
{
	DirectX::XMStoreFloat4x4(&m_Transform, _AccumulatedTranform);
	Entity::Submit(_Filter);
}

DirectX::XMMATRIX Mesh::GetTransformXM() const noexcept
{
	return DirectX::XMLoadFloat4x4(&m_Transform);
}
