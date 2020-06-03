#pragma once
#include "stdafx.h"

namespace custom
{
	class SVDescriptor
	{
	public:
		SVDescriptor(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount)
			: g_pDevice(pDevice)
		{
			m_heapDesc.Type = Type;
			m_heapDesc.NumDescriptors = MaxCount;
			m_heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			m_heapDesc.NodeMask = 1;
		}

		void Create(const std::wstring& DebugHeapName)
		{
			ASSERT_HR(m_pDevice->CreateDescriptorHeap(&m_heapDesc, IID_PPV_ARGS(m_pHeap.ReleaseAndGetAddressOf())));
#ifdef RELEASE
			(void)DebugHeapName;
#else
			m_pHeap->SetName(DebugHeapName.c_str());
#endif

			m_descriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(m_heapDesc.Type);
			m_numFreeDescriptors = m_heapDesc.NumDescriptors;
			m_firstHandle = custom::DescriptorHandle(m_pHeap->GetCPUDescriptorHandleForHeapStart(), m_pHeap->GetGPUDescriptorHandleForHeapStart());
			m_nextFreeHandle = m_firstHandle;
		}

		bool HasAvailableSpace(uint32_t Count) const
		{
			return Count <= m_numFreeDescriptors;
		}

		custom::DescriptorHandle Alloc(uint32_t Count = 1)
		{
			ASSERT(HasAvailableSpace(Count), "Descriptor Heap out of space.  Increase heap size.");
			custom::DescriptorHandle ret = m_nextFreeHandle;
			m_nextFreeHandle += Count * m_descriptorSize;
			return ret;
		}

		custom::DescriptorHandle GetHandleAtOffset(uint32_t Offset) const { return m_firstHandle + Offset * m_descriptorSize; }

		bool ValidateHandle(const custom::DescriptorHandle& DHandle) const
		{
			if ((DHandle.GetCpuHandle().ptr < m_firstHandle.GetCpuHandle().ptr) ||
				(m_firstHandle.GetCpuHandle().ptr + m_heapDesc.NumDescriptors * m_descriptorSize <= DHandle.GetCpuHandle().ptr))
			{
				return false;
			}

			if (DHandle.GetGpuHandle().ptr - m_firstHandle.GetGpuHandle().ptr !=
				DHandle.GetCpuHandle().ptr - m_firstHandle.GetCpuHandle().ptr)
			{
				return false;
			}

			return true;
		}

		ID3D12DescriptorHeap* GetHeapPointer() const
		{
			return m_pHeap.Get();
		}

	private:
		CONTAINED ID3D12Device* m_pDevice;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pHeap;
		D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
		uint32_t m_descriptorSize;
		uint32_t m_numFreeDescriptors;
		custom::DescriptorHandle m_firstHandle;
		custom::DescriptorHandle m_nextFreeHandle;
	};
}