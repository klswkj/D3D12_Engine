#pragma once
#include "stdafx.h"

class SamplerManager
{
	SamplerManager(ID3D12Device* pDevice, DescriptorHeapManager& AllocateManager)
		: m_pDevice(pDevice), m_rAllocateManager(AllocateManager)
	{
	}
	~SamplerManager()
	{
		cleanUp();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RequestHandle(custom::SamplerDescriptor& samplerdesc);

private:
	size_t makeHash(custom::SamplerDescriptor& samplerDesc);
	void cleanUp();

private:
	ID3D12Device* g_pDevice;
	DescriptorHeapManager& m_rAllocateManager;

	std::unordered_map<size_t, D3D12_CPU_DESCRIPTOR_HANDLE> m_samplerDescriptorHashMap;
};