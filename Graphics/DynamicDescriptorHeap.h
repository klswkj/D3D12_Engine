#pragma once
#include "Device.h"
#include "DescriptorHandle.h"

namespace device
{
	extern ID3D12Device* g_pDevice;
}

namespace custom
{
	// This class is a linear allocation system for dynamically generated descriptor tables.  It internally caches
	// CPU descriptor handles so that when not enough space is available in the current heap, necessary descriptors
	// can be re-copied to the new heap.
	class RootSignature;
	class CommandContext;

	class DynamicDescriptorHeap
	{
	public:
		DynamicDescriptorHeap
		(
			custom::CommandContext& OwningContext, 
			D3D12_DESCRIPTOR_HEAP_TYPE HeapType
		)
			: m_owningContext(OwningContext), m_descriptorType(HeapType)
		{
			m_pCurrentHeap = nullptr;
			m_currentOffset = 0;
			m_descriptorSize = device::g_pDevice->GetDescriptorHandleIncrementSize(HeapType);
		}

		~DynamicDescriptorHeap()
		{
		}

		static void DestroyAll(void)
		{
			sm_descriptorHeapPool[0].clear();
			sm_descriptorHeapPool[1].clear();
		}

		void CleanupUsedHeaps(uint64_t fenceValue);

		// Copy multiple handles into the cache area reserved for the specified root parameter.
		void SetGraphicsDescriptorHandles
		(
			uint32_t RootIndex, uint32_t Offset, uint32_t NumHandles,
			const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]
		)
		{
			m_graphicsHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
		}

		void SetComputeDescriptorHandles
		(
			uint32_t RootIndex, uint32_t Offset, uint32_t NumHandles,
			const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]
		)
		{
			m_computeHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
		}

		// Bypass the cache and upload directly to the shader-visible heap
		D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handle);

		// Deduce cache layout needed to support the descriptor tables needed by the root signature.
		void ParseGraphicsRootSignature(const custom::RootSignature& RootSig)
		{
			m_graphicsHandleCache.ParseRootSignature(m_descriptorType, RootSig);
		}

		void ParseComputeRootSignature(const custom::RootSignature& RootSig)
		{
			m_computeHandleCache.ParseRootSignature(m_descriptorType, RootSig);
		}

		// Upload any new descriptors in the cache to the shader-visible heap.
		inline void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
		{
			if (m_graphicsHandleCache.m_staleRootParamsBitMap != 0)
			{
				copyAndBindStagedTables(m_graphicsHandleCache, CmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
			}
		}

		inline void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
		{
			if (m_computeHandleCache.m_staleRootParamsBitMap != 0)
			{
				copyAndBindStagedTables(m_computeHandleCache, CmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
			}
		}

	private:
		// Static methods
		static ID3D12DescriptorHeap* requestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
		static void discardDescriptorHeaps
		(
			D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset, 
			const std::vector<ID3D12DescriptorHeap*>& UsedHeaps
		);

	private:
		bool hasSpace(uint32_t Count = 1)
		{
			return (m_pCurrentHeap != nullptr && m_currentOffset + Count <= kNumDescriptorsPerHeap);
		}

		void retireCurrentHeap();
		void retireUsedHeaps(uint64_t fenceValue);
		ID3D12DescriptorHeap* getHeapPointer();

		DescriptorHandle allocate(UINT Count)
		{
			DescriptorHandle ret = m_firstDescriptor + m_currentOffset * m_descriptorSize;
			m_currentOffset += Count;
			return ret;
		}

		struct DescriptorHandleCache; // Definition at this file line 147.

		void copyAndBindStagedTables
		(
			DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
			void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)
		);

		// Mark all descriptors in the cache as stale and in need of re-uploading.
		void unbindAllValid(void);

	private:
		// Static members
		static const uint32_t kNumDescriptorsPerHeap = 1024;
		static std::mutex sm_mutex;

		static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_descriptorHeapPool[2];
		static std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> sm_retiredDescriptorHeaps[2];
		static std::queue<ID3D12DescriptorHeap*> sm_availableDescriptorHeaps[2];

		// Non-static members
	private:
		const D3D12_DESCRIPTOR_HEAP_TYPE m_descriptorType;
		CommandContext& m_owningContext;

		uint32_t m_descriptorSize;
		uint32_t m_currentOffset;

		ID3D12DescriptorHeap* m_pCurrentHeap;
		DescriptorHandle m_firstDescriptor;
		std::vector<ID3D12DescriptorHeap*> m_pRetiredHeaps;

		// a region of the handle cache and which handles have been set
		struct DescriptorTableCache
		{
			DescriptorTableCache() 
				: assignedHandlesBitMap(0) 
			{
			}

			D3D12_CPU_DESCRIPTOR_HANDLE* pTableStart;
			uint32_t assignedHandlesBitMap;
			uint32_t tableSize;
		};

		struct DescriptorHandleCache
		{
			DescriptorHandleCache()
			{
				ClearCache();
			}

			void ClearCache()
			{
				m_rootDescriptorTablesBitMap = 0;
				m_maxCachedDescriptors = 0;
			}

			uint32_t ComputeStagedSize();

			void CopyAndBindStaleTables
			(
				D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, 
				DescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
				void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)
			);

			void UnbindAllValid();
			void StageDescriptorHandles
			(
				UINT RootIndex, UINT Offset, UINT NumHandles, 
				const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]
			);
			void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const RootSignature& RootSig);

			static constexpr size_t kMaxNumDescriptors = 256;
			static constexpr size_t kMaxNumDescriptorTables = 16;

			uint32_t m_rootDescriptorTablesBitMap;
			uint32_t m_staleRootParamsBitMap;
			uint32_t m_maxCachedDescriptors;

			DescriptorTableCache m_rootDescriptorTable[kMaxNumDescriptorTables];

			D3D12_CPU_DESCRIPTOR_HANDLE m_handleCache[kMaxNumDescriptors];
		};

		DescriptorHandleCache m_graphicsHandleCache;
		DescriptorHandleCache m_computeHandleCache;
	};
}