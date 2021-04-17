#pragma once
#include "d3dx12Residency.h"
#include "CommandContextManager.h"
#include "CommandQueueManager.h"

namespace custom
{
	class D3D12Adapter;
}

namespace custom
{
	class D3D12GPUNode
	{
	private:
		friend D3D12Adapter;
		explicit D3D12GPUNode();

	public:
		custom::D3D12Adapter* GetParentAdapter() const { return m_pParentAdapter; }
		D3DX12Residency::ResidencyManager* GetResidencyManager() { return &m_ResidencyManager; }
		D3D12_NODE_MASK GetGPUNodeIndexMask() const { return m_GPUNodeMask; }
		D3D12_NODE_MASK GetGPUVisibleNodeMask() const { return m_GPUVisibleNodeMask; }

	private:
		D3D12_NODE_MASK                   m_GPUNodeMask; // BitFlag Format *Not* Index 
		D3D12_NODE_MASK                   m_GPUVisibleNodeMask;
		custom::D3D12Adapter*             m_pParentAdapter;
		D3DX12Residency::ResidencyManager m_ResidencyManager;
		CommandContextManager             m_CommandContextManager;
		CommandQueueManager               m_CommandQueueManager;
	};
}