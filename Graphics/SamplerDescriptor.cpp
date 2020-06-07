#include "stdafx.h"
#include "Device.h"
#include "Graphics.h"
#include "SamplerDescriptor.h"

namespace custom
{
	D3D12_CPU_DESCRIPTOR_HANDLE SamplerDescriptor::RequestHandle()
	{
		auto iter = sm_samplerDescriptorHashMap.find(makeHash());
		if (iter != sm_samplerDescriptorHashMap.end())
		{
			return iter->second;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Handle = graphics::g_DescriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		device::g_pDevice->CreateSampler(this, Handle);

		m_finalized = true;

		return Handle;
	}

	size_t SamplerDescriptor::makeHash()
	{
		SamplerDescriptorHash seed(*this);
		return Hash::MakeHash(&seed.Hash0);
	}

	void SamplerDescriptor::cleanUp()
	{
		sm_samplerDescriptorHashMap.clear();
	}
}