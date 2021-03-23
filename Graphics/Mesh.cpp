#include "stdafx.h"
#include "Mesh.h"
#include "FundamentalVertexIndex.h"

#if defined(_DEBUG)
Mesh::Mesh(const Material& CMaterial, FundamentalVertexIndex& Input, const float* pStartVertexLocation, std::string MeshName)
	: IEntity(CMaterial, Input, pStartVertexLocation, MeshName)
#else
Mesh::Mesh(const Material& CMaterial, FundamentalVertexIndex& Input, const float* pStartVertexLocation)
	: IEntity(CMaterial, Input, pStartVertexLocation)
#endif
{
	ZeroMemory(&m_Transform, sizeof(m_Transform));
}

void Mesh::Submit(eObjectFilterFlag _Filter, Math::Matrix4 _AccumulatedTranform) DEBUG_EXCEPT
{
	// DirectX::XMStoreFloat4x4(&m_Transform, _AccumulatedTranform);
	m_Transform = _AccumulatedTranform;
	IEntity::Submit(_Filter);
}

Math::Matrix4 Mesh::GetTransform() const noexcept
{
	// return DirectX::XMLoadFloat4x4(&m_Transform);
	return m_Transform;
}
