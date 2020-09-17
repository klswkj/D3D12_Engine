#pragma once
#include "Entity.h"
#include "UAVBuffer.h"

class FilledPrimitiveSphere : public Entity
{
public:
	FilledPrimitiveSphere(float Radius = 2.0f, uint32_t SliceCount = 20u, uint32_t StackCount = 20u);
	void SetPos(DirectX::XMFLOAT3 Position) noexcept;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
private:
	//custom::StructuredBuffer  m_VerticesBuffer;
	//custom::ByteAddressBuffer m_IndicesBuffer;
	//D3D12_PRIMITIVE_TOPOLOGY m_Topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
};