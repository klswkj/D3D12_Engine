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
		const uint8_t numCommandALs
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

	// 지금 모르는거 
	// SliceIndex가 ArraySlice-Index인지 MipSlice-Index인지 모르겠음.

	STATIC uint64_t CopyContext::InitializeTextureArraySlice(GPUResource& dest, UINT sliceIndex, GPUResource& src)
	{
		custom::CopyContext& copyContext = custom::CopyContext::Begin(1u);
		copyContext.SetResourceTransitionBarrierIndex(0u);

		ASSERT(!copyContext.m_numStandByBarriers);

		ID3D12Resource* const pDestResource = dest.GetResource();
		ID3D12Resource* const pSrcResource  = src.GetResource();
		const D3D12_RESOURCE_DESC& DestDesc = pDestResource->GetDesc();
		const D3D12_RESOURCE_DESC& SrcDesc  = pSrcResource->GetDesc();

		ASSERT
		(
			sliceIndex < DestDesc.DepthOrArraySize &&
			SrcDesc.DepthOrArraySize == 1          &&
			DestDesc.Width == SrcDesc.Width        &&
			DestDesc.Height == SrcDesc.Height      &&
			DestDesc.MipLevels <= SrcDesc.MipLevels
		);

		size_t SubResourceIndex = sliceIndex * (size_t)DestDesc.MipLevels;

		uint32_t StartSubResourceIndex = (UINT)SubResourceIndex;
		uint32_t EndSubResourceIndex   = StartSubResourceIndex + DestDesc.MipLevels - 1;

#if defined(_DEBUG)
		if (32u <= EndSubResourceIndex)
		{
			__debugbreak();
		}
#endif

		// copyContext.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		// [sliceIndex * (size_t)DestDesc.MipLevels, sliceIndex * (size_t)DestDesc.MipLevels + DestDesc.MipLevels - 1]
		// -> 이거를 BitFlag로 어떻게 나타낼꺼니?
		// m_bRecordingFlag &= ~(((1 << (EndALIndex - m_StartCommandALIndex + 1)) - 1) << m_StartCommandALIndex);
		UINT SubResourceBitFlag = (((1 << (EndSubResourceIndex - StartSubResourceIndex + 1)) - 1) << StartSubResourceIndex);
		copyContext.TransitionResources(dest, D3D12_RESOURCE_STATE_COMMON, SubResourceBitFlag);
		copyContext.SubmitResourceBarriers(0u);

		dest.ExplicitPromoteTransition(SubResourceBitFlag, D3D12_RESOURCE_STATE_COPY_DEST);

		UINT InputSubresourceIndex = 0ul;

		for (UINT MipLevel = 0; MipLevel < DestDesc.MipLevels; ++MipLevel)
		{
			InputSubresourceIndex = (UINT)SubResourceIndex + MipLevel;

			D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
			{
				pDestResource,
				D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
				InputSubresourceIndex
			};

			D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
			{
				pSrcResource,
				D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
				MipLevel
			};

			copyContext.m_pCommandLists[0]->CopyTextureRegion(&destCopyLocation, 0U, 0U, 0U, &srcCopyLocation, nullptr);
		}

		ASSERT(!copyContext.m_numStandByBarriers);
		copyContext.TransitionResources(dest, D3D12_RESOURCE_STATE_GENERIC_READ, SubResourceBitFlag);
		copyContext.SubmitResourceBarriers(0u);

		return copyContext.Finish(false);
	}
	STATIC uint64_t CopyContext::InitializeBuffer(GPUResource& dest, const void* pBufferData, size_t numBytes, size_t Offset /* = 0*/)
	{
		CopyContext& InitContext = CopyContext::Begin(1u);
		InitContext.SetResourceTransitionBarrierIndex(0u);

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
		
		InitContext.TransitionResources(dest, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES); // InitContext.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		InitContext.SubmitResourceBarriers(0u);

		dest.ExplicitPromoteTransition(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_DEST);

		InitContext.m_pCommandLists[0]->CopyBufferRegion(dest.GetResource(), Offset, mem.buffer.GetResource(), 0, numBytes);
		// InitContext.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ);
		// InitContext.SubmitResourceBarriers(0u);

		// Execute the command list and wait for it to finish so we can release the upload buffer
		return InitContext.Finish(false);
	} 
	STATIC uint64_t CopyContext::ReadbackTexture2D
	(
		GPUResource& readbackBuffer, PixelBuffer& srcBuffer, 
		const UINT readbackBufferFirstSubResourceIndex, const UINT srcBufferFirstSubResourceIndex, const UINT numSubResources
	)
	{
		std::unique_ptr<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]> pSourceBufferPlacedFootPrint = std::make_unique<D3D12_PLACED_SUBRESOURCE_FOOTPRINT[]>(5);

		ID3D12Resource* pDstResource = readbackBuffer.GetResource();
		ID3D12Resource* pSrcResource = srcBuffer.GetResource();
		{
			D3D12_RESOURCE_DESC PixelBufferResourceDesc = pSrcResource->GetDesc();
			ID3D12Device* pDevice = nullptr;
			pSrcResource->GetDevice(IID_PPV_ARGS(&pDevice));

			pDevice->GetCopyableFootprints(&PixelBufferResourceDesc, srcBufferFirstSubResourceIndex, numSubResources, 0ULL, pSourceBufferPlacedFootPrint.get(), nullptr, nullptr, nullptr);
			pDevice->Release();
		}

		// This very short command list only issues one API call and 
		// will be synchronized so we can immediately read the buffer contents.
		CopyContext& CopyContext = CopyContext::Begin(1u);
		CopyContext.SetResourceTransitionBarrierIndex(0u);
		ASSERT(!CopyContext.m_numStandByBarriers);

		// CopyContext.TransitionResource(srcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CopyContext.TransitionResources(srcBuffer, D3D12_RESOURCE_STATE_COMMON, (UINT)(((1U << numSubResources) - 1U) << srcBufferFirstSubResourceIndex));
		CopyContext.SubmitResourceBarriers(0u);

		for(UINT i = 0; i < numSubResources; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION Dst = CD3DX12_TEXTURE_COPY_LOCATION(pDstResource, *(pSourceBufferPlacedFootPrint.get() + i));
			D3D12_TEXTURE_COPY_LOCATION Src = CD3DX12_TEXTURE_COPY_LOCATION(pSrcResource, srcBufferFirstSubResourceIndex + i);

			CopyContext.m_pCommandLists[0]->CopyTextureRegion
			(
				&Dst, 0U, 0U, 0U,
				&Src, nullptr
			);
		}

		return CopyContext.Finish(false);
	}

	void CopyContext::WriteBuffer(GPUResource& dest, size_t destOffset, const void* pBufferData, size_t numBytes, const uint8_t commandIndex/* = 0u*/)
	{
		ASSERT(pBufferData != nullptr && HashInternal::IsAligned(pBufferData, 16ul) && CHECK_VALID_COMMAND_INDEX);
		LinearBuffer temp = m_CPULinearAllocator.Allocate(numBytes, 512ul);
		SIMDMemCopy(temp.pData, pBufferData, HashInternal::DivideByMultiple(numBytes, 16ul));
		CopyBufferRegion(dest, destOffset, temp.buffer, temp.offset, numBytes, commandIndex);
	}

	void CopyContext::FillBuffer(GPUResource& dest, size_t destOffset, float value, size_t numBytes, const uint8_t commandIndex/* = 0u*/)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		LinearBuffer temp = m_CPULinearAllocator.Allocate(numBytes, 512ul);
		__m128 VectorValue = _mm_set1_ps(value);
		SIMDMemFill(temp.pData, VectorValue, HashInternal::DivideByMultiple(numBytes, 16ul));
		CopyBufferRegion(dest, destOffset, temp.buffer, temp.offset, numBytes, commandIndex);
	}

	void CopyContext::CopyBuffer(GPUResource& dest, GPUResource& src, const uint8_t commandIndex/* = 0u*/)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		TransitionResource(dest, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES); // TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		TransitionResource(src,  D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES); // TransitionResource(src,  D3D12_RESOURCE_STATE_COPY_SOURCE);

		dest.ExplicitPromoteTransition(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_DEST);
		src.ExplicitPromoteTransition(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_SOURCE);

		SubmitResourceBarriers(commandIndex);
		m_pCommandLists[commandIndex]->CopyResource(dest.GetResource(), src.GetResource());
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void CopyContext::CopyBufferRegion
	(
		GPUResource& dest, size_t destOffset,
		GPUResource& src, size_t srcOffset, size_t numBytes, 
		const uint8_t commandIndex
	)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		TransitionResource(dest, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES); // TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
	    TransitionResource(src,  D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES); // TransitionResource(src,  D3D12_RESOURCE_STATE_COPY_SOURCE);
		SubmitResourceBarriers(commandIndex);

		dest.ExplicitPromoteTransition(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_DEST);
		src.ExplicitPromoteTransition(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_SOURCE);

		m_pCommandLists[commandIndex]->CopyBufferRegion(dest.GetResource(), destOffset, src.GetResource(), srcOffset, numBytes);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void CopyContext::CopySubresource
	(
		GPUResource& dest, UINT destSubIndex,
		GPUResource& src, UINT srcSubIndex, 
		const uint8_t commandIndex
	)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		TransitionResource(dest, D3D12_RESOURCE_STATE_COMMON, destSubIndex); // TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		TransitionResource(src,  D3D12_RESOURCE_STATE_COMMON, srcSubIndex); // TransitionResource(src,  D3D12_RESOURCE_STATE_COPY_SOURCE);
		SubmitResourceBarriers(commandIndex);

		dest.ExplicitPromoteTransition(1 << destSubIndex, D3D12_RESOURCE_STATE_COPY_DEST);
		src.ExplicitPromoteTransition(1 << srcSubIndex, D3D12_RESOURCE_STATE_COPY_SOURCE);

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

	void CopyContext::CopyCounter(GPUResource& dest, size_t destOffset, StructuredBuffer& src, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX);
		TransitionResource(dest,                   D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES); // TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		TransitionResource(src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES); // TransitionResource(src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
		SubmitResourceBarriers(commandIndex);

		dest.ExplicitPromoteTransition(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_DEST);
		src.GetCounterBuffer().ExplicitPromoteTransition(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_SOURCE);

		m_pCommandLists[commandIndex]->CopyBufferRegion(dest.GetResource(), destOffset, src.GetCounterBuffer().GetResource(), 0, 4);
		INCRE_DEBUG_ASYNC_THING_COUNT;
	}

	void CopyContext::ResetCounter(StructuredBuffer& buffer, float value, const uint8_t commandIndex)
	{
		ASSERT(CHECK_VALID_COMMAND_INDEX && !m_numStandByBarriers);
		FillBuffer(buffer.GetCounterBuffer(), 0, value, sizeof(float), commandIndex);
		TransitionResource(buffer.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		SubmitResourceBarriers(commandIndex);
	}
}