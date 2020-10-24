#include "stdafx.h"
#include "FundamentalVertexBuffer.h"
#include "CommandContext.h"

FundamentalVertexBuffer::FundamentalVertexBuffer(ID3D12Resource* pResource, D3D12_GPU_VIRTUAL_ADDRESS _BufferLocation, UINT _SizeInBytes, UINT _StrideInBytes)
{

}
void FundamentalVertexBuffer::Bind(custom::CommandContext& BaseContext)
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetVertexBuffer(0, m_VertexBufferView);
}