#pragma once

struct FundamentalVertexIndex
{
	FundamentalVertexIndex() {};
	FundamentalVertexIndex(D3D12_VERTEX_BUFFER_VIEW VBV, D3D12_INDEX_BUFFER_VIEW IBV, UINT _IndexCount, UINT _StartIndexLocation, UINT _VertexCount, INT _StartVertexLocation)
	{
		VertexBufferView = VBV;
		IndexBufferView = IBV;
		IndexCount = _IndexCount;
		StartIndexLocation = _StartIndexLocation;
		VertexCount = _VertexCount;
		BaseVertexLocation = _StartVertexLocation;
	}

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {}; // BufferLocation( D3D12_GPU_VIRTUAL_ADDRESS )
	                                           // SizeInBytes
	                                           // StrideInBytes

	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};   // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
	                                           // UINT SizeInBytes;
	                                           // DXGI_FORMAT Format;
	UINT IndexCount = -1;
	UINT StartIndexLocation = -1;
	INT BaseVertexLocation = -1;

	UINT VertexCount = -1; // For Computing Bounding Box.
};
