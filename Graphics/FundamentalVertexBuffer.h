#pragma once
#include "RenderingResource.h"

namespace custom
{
	class CommandContext;
}

class FundamentalVertexBuffer : public RenderingResource
{
public:
	FundamentalVertexBuffer(ID3D12Resource* pResource, D3D12_GPU_VIRTUAL_ADDRESS _BufferLocation, UINT _SizeInBytes, UINT _StrideInBytes);
	void Bind(custom::CommandContext& BaseContext);
private:
	ID3D12Resource* m_pD3D12Resource;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView; // BufferLocation( D3D12_GPU_VIRTUAL_ADDRESS )
	                                             // SizeInBytes
	                                             // StrideInBytes
};