#pragma once
#include "RenderingResource.h"

namespace custom
{
	class GPUResource;

	class ResourceBarrier final : public RenderingResource
	{
	public:
		explicit ResourceBarrier();
		void Bind(custom::CommandContext& baseContext, uint8_t commandIndex) DEBUG_EXCEPT final;

		void InsertResourceUAVBarrier(GPUResource& resource, uint8_t barrierIndex);
		void InsertResourceAliasingBarrier(GPUResource& resource, GPUResource& after, uint8_t barrierIndex);
		void InsertResourceTransitionBarrier(GPUResource& resource, const D3D12_RESOURCE_STATES newState, const uint8_t barrierIndex, UINT subResources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	
	private:
		D3D12_RESOURCE_BARRIER m_ResourceBarrier[16];
		uint8_t m_NumBarrier;
	};

}