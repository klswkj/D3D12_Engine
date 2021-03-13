#pragma once
#include "CommandContext.h"

namespace custom
{
	class GPUResource;
	class StructuredBuffer;

	class CopyContext final : public CommandContext
	{
	public:
		static CopyContext& Begin(const uint8_t numCommandALs = 1u);
		static CopyContext& Resume(const uint64_t contextID);
		static CopyContext& GetRecentContext();
		static uint64_t InitializeTexture(GPUResource& dest, UINT numSubresources, D3D12_SUBRESOURCE_DATA subDataArray[]);
		static uint64_t InitializeTextureArraySlice(GPUResource& dest, UINT sliceIndex, GPUResource& src);
		static uint64_t InitializeBuffer(GPUResource& dest, const void* pBufferData, size_t numBytes, size_t offset = 0ul);
		static uint64_t ReadbackTexture2D(GPUResource& readbackBuffer, PixelBuffer& srcBuffer);

		void WriteBuffer(GPUResource& dest, size_t destOffset, const void* BufferData, size_t numBytes, const uint8_t commandIndex = 0u);
		void FillBuffer(GPUResource& dest, size_t destOffset, float value, size_t numBytes, const uint8_t commandIndex = 0u);
		void CopyBuffer(GPUResource& dest, GPUResource& src, const uint8_t commandIndex = 0u);
		void CopyBufferRegion
		(
			GPUResource& dest, size_t destOffset,
			GPUResource& src, size_t srcOffset, size_t numBytes, 
			const uint8_t commandIndex = 0u
		);
		void CopySubresource
		(
			GPUResource& dest, UINT destSubIndex,
			GPUResource& src, UINT srcSubIndex, 
			const uint8_t commandIndex = 0u
		);
		void CopyCounter(GPUResource& dest, size_t destOffset, StructuredBuffer& src, const uint8_t commandIndex = 0u);
		void ResetCounter(StructuredBuffer& Buf, float value = 0, const uint8_t commandIndex = 0u);
	private:
	};
}