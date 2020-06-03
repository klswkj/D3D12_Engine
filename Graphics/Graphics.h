#pragma once
/*

   App ¦£ Graphics ¦£ GraphicsDeviceManager ( ID3D12, IDXGI, IWIC, ID2D, ... )
	   ¦¢          ¦¢ DescriptorHeapManager( Descriptor Heap Manager)
	   ¦¢          ¦¦ SamplerManager( custom::SamplerDescriptor Manage(hash, request allocate) )
	   ¦¢ 
	   ¦¦ Windows
*/

class Graphics
{
public:
	void Init();

private:
	GraphicsDeviceManager GRPXDeviceManager;

	DescriptorHeapAllocator DescriptorHeapManager[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};
	custom::SamplerDescriptor samplerManager;
};