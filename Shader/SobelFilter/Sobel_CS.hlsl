//=============================================================================
// Performs edge detection using Sobel operator.
//=============================================================================

Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);


// Approximates luminance ("brightness") from an RGB value.  These weights are derived from
// experiment based on eye sensitivity to different wavelengths of light.
float CalcLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

[numthreads(16, 16, 1)]
void SobelCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    // Sample the pixels in the neighborhood of this pixel.
	float4 c[3][3];
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
		{
			int2 xy = dispatchThreadID.xy + int2(-1 + j, -1 + i);
			c[i][j] = gInput[xy]; 
		}
	}

	// For each color channel, estimate partial x derivative using Sobel scheme.
	float4 Gx = -1.0f*c[0][0] - 2.0f*c[1][0] - 1.0f*c[2][0] + 1.0f*c[0][2] + 2.0f*c[1][2] + 1.0f*c[2][2];

	// For each color channel, estimate partial y derivative using Sobel scheme.
	float4 Gy = -1.0f*c[2][0] - 2.0f*c[2][1] - 1.0f*c[2][1] + 1.0f*c[0][0] + 2.0f*c[0][1] + 1.0f*c[0][2];

	// Gradient is (Gx, Gy).  For each color channel, compute magnitude to get maximum rate of change.
	float4 mag = sqrt(Gx*Gx + Gy*Gy);

	// Make edges black, and nonedges white.
	mag = 1.0f - saturate(CalcLuminance(mag.rgb));

	gOutput[dispatchThreadID.xy] = mag;
}

/*
#include "SobelFilter.h"
 
SobelFilter::SobelFilter(ID3D12Device* device, 
	                   UINT width, UINT height,
                       DXGI_FORMAT format)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;
	mFormat = format;

	BuildResource();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SobelFilter::OutputSrv()
{
	return mhGpuSrv;
}

UINT SobelFilter::DescriptorCount()const
{
	return 2;
}

void SobelFilter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
	                              CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
	                              UINT descriptorSize)
{
	// Save references to the descriptors. 
	mhCpuSrv = hCpuDescriptor;
	mhCpuUav = hCpuDescriptor.Offset(1, descriptorSize);
	mhGpuSrv = hGpuDescriptor;
	mhGpuUav = hGpuDescriptor.Offset(1, descriptorSize);

	BuildDescriptors();
}

void SobelFilter::OnResize(UINT newWidth, UINT newHeight)
{
	if((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();

		// New resource, so we need new descriptors to that resource.
		BuildDescriptors();
	}
}
 
void SobelFilter::Execute(ID3D12GraphicsCommandList* cmdList, 
	                     ID3D12RootSignature* rootSig,
	                     ID3D12PipelineState* pso,
	                     CD3DX12_GPU_DESCRIPTOR_HANDLE input)
{
	cmdList->SetComputeRootSignature(rootSig);
	cmdList->SetPipelineState(pso);

	cmdList->SetComputeRootDescriptorTable(0, input);
	cmdList->SetComputeRootDescriptorTable(2, mhGpuUav);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutput.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	// How many groups do we need to dispatch to cover image, where each
	// group covers 16x16 pixels.
	UINT numGroupsX = (UINT)ceilf(mWidth / 16.0f);
	UINT numGroupsY = (UINT)ceilf(mHeight / 16.0f);
	cmdList->Dispatch(numGroupsX, numGroupsY, 1);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutput.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void SobelFilter::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = mFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	md3dDevice->CreateShaderResourceView(mOutput.Get(), &srvDesc, mhCpuSrv);
	md3dDevice->CreateUnorderedAccessView(mOutput.Get(), nullptr, &uavDesc, mhCpuUav);
}

void SobelFilter::BuildResource()
{
	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed
	(
		md3dDevice->CreateCommittedResource
		(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mOutput)
		)
	);
}
*/

/*
class SobelFilter
{
public:
	///<summary>
	/// The width and height should match the dimensions of the input texture to apply the filter.
	/// Recreate when the screen is resized. 
	///</summary>
	SobelFilter
	(
		ID3D12Device* device,
		UINT width, UINT height,
		DXGI_FORMAT format);
	
	SobelFilter(const SobelFilter& rhs)=delete;
	SobelFilter& operator=(const SobelFilter& rhs)=delete;
	~SobelFilter()=default;

	CD3DX12_GPU_DESCRIPTOR_HANDLE OutputSrv();

	UINT DescriptorCount()const;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor, 
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
		UINT descriptorSize);

	void OnResize(UINT newWidth, UINT newHeight);

	void Execute(
		ID3D12GraphicsCommandList* cmdList, 
		ID3D12RootSignature* rootSig,
		ID3D12PipelineState* pso,
		CD3DX12_GPU_DESCRIPTOR_HANDLE input);

private:
	void BuildDescriptors();
	void BuildResource();

private:

	ID3D12Device* md3dDevice = nullptr;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuUav;

	Microsoft::WRL::ComPtr<ID3D12Resource> mOutput = nullptr;
};

 
*/