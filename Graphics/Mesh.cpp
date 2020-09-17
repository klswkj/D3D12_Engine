#include "stdafx.h"
#include "Mesh.h"
#include "ObjectFilterFlag.h"

Mesh::Mesh(const Material& _Material, const aiMesh& mesh, float scale) DEBUG_EXCEPT
	: Entity(_Material, mesh, scale)
{
	ZeroMemory(&transform, sizeof(transform));
}

void Mesh::Submit(eObjectFilterFlag Filter, DirectX::FXMMATRIX AccumulatedTranform) const DEBUG_EXCEPT
{
	DirectX::XMStoreFloat4x4(&transform, AccumulatedTranform);
	Entity::Submit(Filter);
}

DirectX::XMMATRIX Mesh::GetTransformXM() const noexcept
{
	return DirectX::XMLoadFloat4x4(&transform);
}
