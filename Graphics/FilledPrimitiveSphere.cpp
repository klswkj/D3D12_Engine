#include "stdafx.h"
#include "FilledPrimitiveSphere.h"

FilledPrimitiveSphere::FilledPrimitiveSphere(float Radius /*= 2.0f*/, uint32_t SliceCount /*= 20u*/, uint32_t StackCount /*= 20u*/)
{
	ASSERT(5 <= SliceCount);
	ASSERT(SliceCount <= 20);

	ASSERT(5 <= StackCount);
	ASSERT(StackCount <= 20);

	const size_t VerticesSize = (SliceCount + 1) * (StackCount - 1) + 2;
	const size_t IndicesSize = SliceCount * 6 * (StackCount - 1);

	unsigned short* pVertices = (unsigned short*)malloc(sizeof(float) * VerticesSize); // sizeof(float)가 아니고, sizeof(float * Vertex의 Stride)
	unsigned short* pIndices = (unsigned short*)malloc(sizeof(unsigned short) * IndicesSize);
	/*
	// short
	for (uint32_t i{ 1 }; i <= SliceCount; ++i)
	{
		pIndices32.push_back(0);
		pIndices32.push_back(i + 1);
		pIndices32.push_back(i);
	}
	*/
}

void FilledPrimitiveSphere::SetPos(DirectX::XMFLOAT3 Position) noexcept
{

}

DirectX::XMMATRIX FilledPrimitiveSphere::GetTransformXM() const noexcept
{
	return DirectX::XMMATRIX{};
}