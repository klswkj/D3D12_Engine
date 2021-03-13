#pragma once
#include "Device.h"
#include "DescriptorHandle.h"
#include "CustomCriticalSection.h"

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
			custom::CommandContext& owningContext,
			const D3D12_DESCRIPTOR_HEAP_TYPE heapType
		)
			: 
			m_owningContext(owningContext), 
			m_descriptorType(heapType)
		{
			m_pCurrentHeap = nullptr;
			m_currentOffset = 0;
			m_descriptorSize = device::g_pDevice->GetDescriptorHandleIncrementSize(heapType);
		}

		~DynamicDescriptorHeap()
		{
		}

		static void DestroyAll()
		{
			sm_descriptorHeapPool[0].clear();
			sm_descriptorHeapPool[1].clear();
		}

		void CleanupUsedHeaps(const uint64_t fenceValue);

		// Copy multiple handles into the cache area reserved for the specified root parameter.
		void SetGraphicsDescriptorHandles
		(
			const uint32_t rootIndex, const uint32_t offset, const uint32_t numHandles,
			const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandles[]
		)
		{
			m_graphicsHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, CPUHandles);
		}

		void SetComputeDescriptorHandles
		(
			const uint32_t rootIndex, const uint32_t offset, const uint32_t numHandles,
			const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandles[]
		)
		{
			m_computeHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, CPUHandles);
		}

		// Bypass the cache and upload directly to the shader-visible heap
		D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(const D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, const uint8_t commandIndex);

		// Deduce cache layout needed to support the descriptor tables needed by the root signature.
		void ParseGraphicsRootSignature(const custom::RootSignature& customRS)
		{
			m_graphicsHandleCache.ParseRootSignature(m_descriptorType, customRS);
		}
		void ParseComputeRootSignature(const custom::RootSignature& customRS)
		{
			m_computeHandleCache.ParseRootSignature(m_descriptorType, customRS);
		}

		// Upload any new descriptors in the cache to the shader-visible heap.
		inline void SetGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* const pCmdList, const uint8_t commandIndex)
		{
			if (m_graphicsHandleCache.m_staleRootParamsBitMap != 0)
			{
				SetCachedRootDescriptorTables(m_graphicsHandleCache, pCmdList, commandIndex, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
			}
		}
		inline void SetComputeRootDescriptorTables(ID3D12GraphicsCommandList* const pCmdList, const uint8_t commandIndex)
		{
			if (m_computeHandleCache.m_staleRootParamsBitMap != 0)
			{
				SetCachedRootDescriptorTables(m_computeHandleCache, pCmdList, commandIndex, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
			}
		}

	private:
		// Static methods
		static ID3D12DescriptorHeap* requestDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		static void discardDescriptorHeaps
		(
			const D3D12_DESCRIPTOR_HEAP_TYPE heapType, const uint64_t fenceValueForReset,
			const std::vector<ID3D12DescriptorHeap*>& usedHeaps
		);

	private:
		bool hasSpace(const uint32_t count = 1)
		{
			return (m_pCurrentHeap != nullptr && m_currentOffset + count <= kNumDescriptorsPerHeap);
		}

		void retireCurrentHeap();
		void retireUsedHeaps(const uint64_t fenceValue);
		ID3D12DescriptorHeap* getHeapPointer();

		DescriptorHandle allocateDescriptorHandle(const UINT count)
		{
			DescriptorHandle ret = m_firstDescriptor + m_currentOffset * m_descriptorSize;
			m_currentOffset += count;
			return ret;
		}

		struct DescriptorHandleCache; // Definition at this file line 147.

		void SetCachedRootDescriptorTables
		(
			DescriptorHandleCache& handleCache, ID3D12GraphicsCommandList* const pCmdList, const uint8_t commandIndex,
			void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* pSetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)
		);

		// Mark all descriptors in the cache as stale and in need of re-uploading.
		void unbindAllValid();

	private:
		// Static members
		static const uint32_t kNumDescriptorsPerHeap = 1024u;
		// static std::mutex sm_mutex;
		static custom::RAII_CS sm_CS;

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

		// a region of the CPUhandle cache and which handles have been set
		struct DescriptorTableCache
		{
			DescriptorTableCache() 
				: 
				pTableStart(nullptr), 
				assignedHandlesBitMap(0), 
				tableSize(0)
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
				m_rootDescriptorTablesBitMap = 0u;
				m_maxCachedDescriptors = 0u;
			}

			uint32_t ComputeStagedSize() const;

			void SetOwnedRootDescriptorTables
			(
				const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t descriptorSize,
				DescriptorHandle destHandleStart, ID3D12GraphicsCommandList* const pCmdList,
				void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* pSetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)
			);

			void UnbindAllValid();
			void StageDescriptorHandles
			(
				const UINT rootIndex, const UINT offset, const UINT numHandles,
				const D3D12_CPU_DESCRIPTOR_HANDLE handles[]
			);
			void ParseRootSignature(const D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& customRS);

			static constexpr size_t kMaxNumDescriptors = 256ul;
			static constexpr size_t kMaxNumDescriptorTables = 16ul;

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