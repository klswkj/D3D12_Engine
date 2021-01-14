#include "stdafx.h"
#include "Device.h"
#include "UAVBuffer.h"
#include "CommandContext.h"
#include "DescriptorHeapManager.h"

namespace custom
{
	std::map<std::wstring, StructuredBuffer*> s_VertexBufferCache;
	std::map<std::wstring, ByteAddressBuffer*> s_IndexBufferCache;

	std::map<std::wstring, UAVBuffer*> s_VertexBufferCache2;
	std::map<std::wstring, UAVBuffer*> s_IndexBufferCache2;

	void StructuredBuffer::DestroyAllVertexBuffer() // static
	{
		for (auto& e : s_VertexBufferCache)
		{
			e.second->Destroy();
			delete e.second;
		}
		for (auto& e : s_VertexBufferCache2)
		{
			e.second->Destroy();
			delete e.second;
		}

		s_VertexBufferCache.clear();
		s_VertexBufferCache2.clear();
	}

	void ByteAddressBuffer::DestroyIndexAllBuffer() // static
	{
		for (auto& e : s_IndexBufferCache)
		{
			e.second->Destroy();
			delete e.second;
		}
		for (auto& e : s_IndexBufferCache2)
		{
			e.second->Destroy();
			delete e.second;
		}

		s_IndexBufferCache.clear();
		s_IndexBufferCache2.clear();
	}

	void UAVBuffer::Create
	(
		const std::wstring& name,
		uint32_t NumElements, uint32_t ElementSize,
		const void* initialData
	)
	{
		Destroy();

		m_elementCount = NumElements;
		m_elementSize  = ElementSize;
		m_bufferSize   = NumElements * ElementSize;

		D3D12_RESOURCE_DESC ResourceDesc = resourceDescriptor();

		m_currentState = D3D12_RESOURCE_STATE_COMMON;

		D3D12_HEAP_PROPERTIES HeapProps;
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		ASSERT_HR
		(
			device::g_pDevice->CreateCommittedResource
			(
				&HeapProps, D3D12_HEAP_FLAG_NONE,
				&ResourceDesc, m_currentState, nullptr, IID_PPV_ARGS(&m_pResource)
			)
		);

		m_GPUVirtualAddress = m_pResource->GetGPUVirtualAddress();

		if (initialData)
		{
			CommandContext::InitializeBuffer(*this, initialData, m_bufferSize);
		}

#if defined(_DEBUG)
		m_pResource->SetName(name.c_str());
		
#endif
		CreateUAV(name);
	}

	// Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
	void UAVBuffer::CreatePlaced
	(
		const std::wstring& name,
		ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
		const void* initialData
	)
	{
		m_elementCount = NumElements;
		m_elementSize  = ElementSize;
		m_bufferSize   = NumElements * ElementSize;

		D3D12_RESOURCE_DESC ResourceDesc = resourceDescriptor();

		m_currentState = D3D12_RESOURCE_STATE_COMMON;

		ASSERT_HR
		(
			device::g_pDevice->CreatePlacedResource
			(
				pBackingHeap, HeapOffset, &ResourceDesc, m_currentState, nullptr, IID_PPV_ARGS(&m_pResource)
			)
		);

		m_GPUVirtualAddress = m_pResource->GetGPUVirtualAddress();

		if (initialData)
		{
			CommandContext::InitializeBuffer(*this, initialData, m_bufferSize);
		}

#ifdef RELEASE
		(m_Name);
#else
		m_pResource->SetName(name.c_str());
		// pBackingHeap->SetName(name.c_str());
#endif
		CreateUAV(name);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE UAVBuffer::CreateConstantBufferView(uint32_t Offset, uint32_t Size) const
	{
		ASSERT(Offset + Size <= m_bufferSize);

		Size = HashInternal::AlignUp(Size, 16);

		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = m_GPUVirtualAddress + (size_t)Offset;
		CBVDesc.SizeInBytes = Size;

		D3D12_CPU_DESCRIPTOR_HANDLE hCBV = device::g_descriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		device::g_pDevice->CreateConstantBufferView(&CBVDesc, hCBV);
		return hCBV;
	}

	D3D12_RESOURCE_DESC UAVBuffer::resourceDescriptor()
	{
		ASSERT(m_bufferSize != 0);

		D3D12_RESOURCE_DESC Desc = {};
		Desc.Alignment = 0;
		Desc.DepthOrArraySize = 1;
		Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Desc.Flags = m_resourceFlags;
		Desc.Format = DXGI_FORMAT_UNKNOWN;
		Desc.Height = 1;
		Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Desc.MipLevels = 1;
		Desc.SampleDesc.Count = 1;
		Desc.SampleDesc.Quality = 0;
		Desc.Width = (UINT64)m_bufferSize;
		return Desc;
	}
	
	void ByteAddressBuffer::CreateUAV(std::wstring Name)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Buffer.NumElements = (UINT)m_bufferSize / 4;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_SRV = device::g_descriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		device::g_pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, m_SRV);

		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		UAVDesc.Buffer.NumElements = (UINT)m_bufferSize / 4;
		UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

		if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_UAV = device::g_descriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		device::g_pDevice->CreateUnorderedAccessView(m_pResource, nullptr, &UAVDesc, m_UAV);
	}

	void StructuredBuffer::CreateUAV(std::wstring Name)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Buffer.NumElements = m_elementCount;
		SRVDesc.Buffer.StructureByteStride = m_elementSize;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_SRV = device::g_descriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		device::g_pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, m_SRV);

		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		UAVDesc.Buffer.CounterOffsetInBytes = 0;
		UAVDesc.Buffer.NumElements = m_elementCount;
		UAVDesc.Buffer.StructureByteStride = m_elementSize;
		UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		m_CounterBuffer.Create(Name + L"_StructuredBuffer::Counter", 1, 4);

		if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_UAV = device::g_descriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		device::g_pDevice->CreateUnorderedAccessView(m_pResource, m_CounterBuffer.GetResource(), &UAVDesc, m_UAV);
	}

	void TypedBuffer::CreateUAV(std::wstring Name)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SRVDesc.Format = m_DataFormat;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Buffer.NumElements = m_elementCount;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_SRV = device::g_descriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		device::g_pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, m_SRV);

		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		UAVDesc.Format = m_DataFormat;
		UAVDesc.Buffer.NumElements = m_elementCount;
		UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_UAV = device::g_descriptorHeapManager.Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		device::g_pDevice->CreateUnorderedAccessView(m_pResource, nullptr, &UAVDesc, m_UAV);
	}

	StructuredBuffer* StructuredBuffer::CreateVertexBuffer
	(
		const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
		const void* initialData
	)
	{
		{
			static std::mutex s_Mutex;
			std::lock_guard<std::mutex> Guard(s_Mutex);

			auto iter = s_VertexBufferCache.find(name);
			if (iter != s_VertexBufferCache.end())
			{
				// return iter->second.get();
				return iter->second;
			}
		}

		StructuredBuffer* NewVertexBuffer = new StructuredBuffer();

		NewVertexBuffer->Create(name, NumElements, ElementSize, initialData);

		static std::mutex s_Mutex;
		std::lock_guard<std::mutex> Guard(s_Mutex);

		// s_VertexBufferCache[name].reset(NewVertexBuffer);
		s_VertexBufferCache[name] = NewVertexBuffer;
		return NewVertexBuffer;
	}

	ByteAddressBuffer* ByteAddressBuffer::CreateIndexBuffer
	(
		const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
		const void* initialData
	)
	{
		{
			static std::mutex s_Mutex;
			std::lock_guard<std::mutex> Guard(s_Mutex);

			auto iter = s_IndexBufferCache.find(name);
			if (iter != s_IndexBufferCache.end())
			{
				// return iter->second.get();
				return iter->second;
			}
		}

		ByteAddressBuffer* NewIndexBuffer = new ByteAddressBuffer();
		NewIndexBuffer->Create(name, NumElements, ElementSize, initialData);

		static std::mutex s_Mutex;
		std::lock_guard<std::mutex> Guard(s_Mutex);

		// s_IndexBufferCache[name].reset(NewIndexBuffer);
		s_IndexBufferCache[name] = NewIndexBuffer;
		return NewIndexBuffer;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::GetCounterSRV(CommandContext& Context)
	{
		Context.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		return m_CounterBuffer.GetSRV();
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::GetCounterUAV(CommandContext& Context)
	{
		Context.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		return m_CounterBuffer.GetUAV();
	}
}