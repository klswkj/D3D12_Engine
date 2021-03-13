#include "stdafx.h"
#include "CopyContext.h"
#include "Device.h"
#include "CommandContextManager.h"
#include "CommandQueueManager.h"
#include "PixelBuffer.h"
#include "UAVBuffer.h"

namespace custom
{
	STATIC CopyContext& CopyContext::Begin
	(
		const uint8_t numCommandALs /*= 1*/
	)
	{
		ASSERT(numCommandALs < UCHAR_MAX);
		CopyContext* NewContext = reinterpret_cast<CopyContext*>(device::g_commandContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_COPY, numCommandALs));
		return *NewContext;
	}
	STATIC CopyContext& CopyContext::Resume(const uint64_t contextID)
	{
		custom::CommandContext* ret = nullptr;
		ret = device::g_commandContextManager.GetRecordingContext(D3D12_COMMAND_LIST_TYPE_COPY, contextID);
		ASSERT(ret&& ret->GetType() == D3D12_COMMAND_LIST_TYPE_COPY);
		return *reinterpret_cast<custom::CopyContext*>(ret);
	}
	STATIC CopyContext& CopyContext::GetRecentContext()
	{
		custom::CommandContext* ret = nullptr;
		ret = device::g_commandContextManager.GetRecentContext(D3D12_COMMAND_LIST_TYPE_COPY);
		ASSERT(ret && ret->GetType() == D3D12_COMMAND_LIST_TYPE_COPY);
		return *reinterpret_cast<custom::CopyContext*>(ret);
	}

	STATIC uint64_t CopyContext::InitializeTexture(GPUResource& dest, UINT numSubresources, D3D12_SUBRESOURCE_DATA subDataArray[])
	{
		custom::CopyContext& copyContext = custom::CopyContext::Begin(1u);
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(dest.GetResource(), 0U, numSubresources);

		// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
		LinearBuffer mem = copyContext.ReserveUploadMemory(uploadBufferSize);
		UpdateSubresources(copyContext.m_pCommandLists[0], dest.GetResource(), mem.buffer.GetResource(), 0, 0, numSubresources, subDataArray);
		ASSERT(!copyContext.m_numStandByBarriers);
		// copyContext.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ);
		// copyContext.SubmitResourceBarriers(0u);
		// Execute the command list and wait for it to finish so we can release the upload buffer

		return copyContext.Finish(false);
	}
	STATIC uint64_t CopyContext::InitializeTextureArraySlice(GPUResource& dest, UINT sliceIndex, GPUResource& src)
	{
		custom::CopyContext& copyContext = custom::CopyContext::Begin(1u);

		ASSERT(!copyContext.m_numStandByBarriers);
		copyContext.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		copyContext.SubmitResourceBarriers(0u);

		const D3D12_RESOURCE_DESC& DestDesc = dest.GetResource()->GetDesc();
		const D3D12_RESOURCE_DESC& SrcDesc = src.GetResource()->GetDesc();

		ASSERT
		(
			sliceIndex < DestDesc.DepthOrArraySize&&
			SrcDesc.DepthOrArraySize == 1 &&
			DestDesc.Width == SrcDesc.Width &&
			DestDesc.Height == SrcDesc.Height &&
			DestDesc.MipLevels <= SrcDesc.MipLevels
		);

		size_t SubResourceIndex = sliceIndex * (size_t)DestDesc.MipLevels;

		for (UINT MipLevel = 0; MipLevel < DestDesc.MipLevels; ++MipLevel)
		{
			size_t InputSubresourceIndex = SubResourceIndex + (size_t)MipLevel;

			D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
			{
				dest.GetResource(),
				D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
				(UINT)InputSubresourceIndex
			};

			D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
			{
				src.GetResource(),
				D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
				MipLevel
			};

			copyContext.m_pCommandLists[0]->CopyTextureRegion(&destCopyLocation, 0U, 0U, 0U, &srcCopyLocation, nullptr);
		}

		ASSERT(!copyContext.m_numStandByBarriers);
		copyContext.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ);
		copyContext.SubmitResourceBarriers(0u);

		return copyContext.Finish(false);
	}
	STATIC uint64_t CopyContext::InitializeBuffer(GPUResource& dest, const void* pBufferData, size_t numBytes, size_t Offset /* = 0*/)
	{
		CopyContext& InitContext = CopyContext::Begin(1u);

		LinearBuffer mem = InitContext.ReserveUploadMemory(numBytes);

		// TODO : This part can be Optimized.
		if (HashInternal::IsAligned(numBytes, 16) && HashInternal::IsAligned(pBufferData, 16))
		{
			SIMDMemCopy(mem.pData, pBufferData, HashInternal::DivideByMultiple(numBytes, 16));
		}
		else
		{
			memcpy_s(mem.pData, numBytes, pBufferData, numBytes);
		}

		ASSERT(!InitContext.m_numStandByBarriers);
		// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
		InitContext.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		InitContext.SubmitResourceBarriers(0u);
		InitContext.m_pCommandLists[0]->CopyBufferRegion(dest.GetResource(), Offset, mem.buffer.GetResource(), 0, numBytes);
		// InitContext.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ);
		// InitContext.SubmitResourceBarriers(0u);

		// Execute the command list and wait for it to finish so we can release the upload buffer
		return InitContext.Finish(false);
	}
	STATIC uint64_t CopyContext::ReadbackTexture2D(GPUResource& readbackBuffer, PixelBuffer& srcBuffer)
	{
		// The footprint may depend on the device of the resource, but we assume there is only one device.
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
		device::g_pDevice->GetCopyableFootprints(&srcBuffer.GetResource()->GetDesc(), 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

		// This very short command list only issues one API call and 
		// will be synchronized so we can immediately read the buffer contents.
		CopyContext& CopyContext = CopyContext::Begin(1u);

		ASSERT(!CopyContext.m_numStandByBarriers);
		CopyContext.TransitionResource(srcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CopyContext.SubmitResourceBarriers(0u);
		CopyContext.m_pCommandLists[0]->CopyTextureRegion
		(
			&CD3DX12_TEXTURE_COPY_LOCATION(readbackBuffer.GetResource(), PlacedFootprint), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(srcBuffer.GetResource(), 0), nullptr
		);

		return CopyContext.Finish(false);
	}

	void CopyContext::WriteBuffer(GPUResource& dest, size_t destOffset, const void* pBufferData, size_t numBytes, const uint8_t commandIndex/* = 0u*/)
	{
		ASSERT(pBufferData != nullptr && HashInternal::IsAligned(pBufferData, 16) && CHECK_VALID_COMMAND_INDEX);
		LinearBuffer temp = m_CPULinearAllocator.Allocate(numBytes, 512);
		SIMDMemCopy(temp.pData, pBufferData, HashInternal::DivideByMultiple(numBytes, 16));
		CopyBufferRegion(dest, destOffset, temp.buffer, temp.offset, numBytes, commandIndex);
	}

	void CopyContext::FillBuffer(GPUResource& dest, size_t destOffset, float value, size_t numBytes, const uint8_t commandIndex/* = 0u*/)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		LinearBuffer temp = m_CPULinearAllocator.Allocate(numBytes, 512);
		__m128 VectorValue = _mm_set1_ps(value);
		SIMDMemFill(temp.pData, VectorValue, HashInternal::DivideByMultiple(numBytes, 16));
		CopyBufferRegion(dest, destOffset, temp.buffer, temp.offset, numBytes, commandIndex);
	}

	void CopyContext::CopyBuffer(GPUResource& dest, GPUResource& src, const uint8_t commandIndex/* = 0u*/)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		TransitionResource(src,  D3D12_RESOURCE_STATE_COPY_SOURCE);
		SubmitResourceBarriers(commandIndex);
		m_pCommandLists[commandIndex]->CopyResource(dest.GetResource(), src.GetResource());
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void CopyContext::CopyBufferRegion
	(
		GPUResource& dest, size_t destOffset,
		GPUResource& src, size_t srcOffset, size_t numBytes, 
		const uint8_t commandIndex/* = 0u*/
	)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		TransitionResource(src,  D3D12_RESOURCE_STATE_COPY_SOURCE);
		SubmitResourceBarriers(commandIndex);
		m_pCommandLists[commandIndex]->CopyBufferRegion(dest.GetResource(), destOffset, src.GetResource(), srcOffset, numBytes);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void CopyContext::CopySubresource
	(
		GPUResource& dest, UINT destSubIndex,
		GPUResource& src, UINT srcSubIndex, 
		const uint8_t commandIndex/* = 0u*/
	)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		TransitionResource(src,  D3D12_RESOURCE_STATE_COPY_SOURCE);
		SubmitResourceBarriers(commandIndex);

		D3D12_TEXTURE_COPY_LOCATION DestLocation =
		{
			dest.GetResource(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			destSubIndex
		};

		D3D12_TEXTURE_COPY_LOCATION SrcLocation =
		{
			src.GetResource(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			srcSubIndex
		};

		m_pCommandLists[commandIndex]->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void CopyContext::CopyCounter(GPUResource& dest, size_t destOffset, StructuredBuffer& src, const uint8_t commandIndex/* = 0u*/)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		TransitionResource(src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
		SubmitResourceBarriers(commandIndex);
		m_pCommandLists[commandIndex]->CopyBufferRegion(dest.GetResource(), destOffset, src.GetCounterBuffer().GetResource(), 0, 4);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void CopyContext::ResetCounter(StructuredBuffer& buffer, float value, const uint8_t commandIndex/* = 0u*/)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX && !m_numStandByBarriers);
		FillBuffer(buffer.GetCounterBuffer(), 0, value, sizeof(float), commandIndex);
		TransitionResource(buffer.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		SubmitResourceBarriers(commandIndex);
	}
}