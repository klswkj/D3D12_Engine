#pragma once
#include "Entity.h"
#include "ObjectFilterFlag.h"

class Material;
struct aiMesh;
struct FundamentalVertexIndex;

class Mesh : public Entity
{
	struct BoundingBox;
public:
	Mesh(const Material& CMaterial, FundamentalVertexIndex& Input, const float* pStartVertexLocation);

	// DirectX::XMMATRIX GetTransformXM() const noexcept;
	Math::Matrix4 GetTransform() const noexcept;

	// void Submit(eObjectFilterFlag Filter, DirectX::FXMMATRIX AccumulatedTranform) const DEBUG_EXCEPT;
	void Submit(eObjectFilterFlag Filter, Math::Matrix4 AccumulatedTranform) const DEBUG_EXCEPT;
private:
	// mutable DirectX::XMFLOAT4X4 m_Transform;
	mutable Math::Matrix4 m_Transform;
};