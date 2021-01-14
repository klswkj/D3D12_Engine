#pragma once
#include "stdafx.h"
#include "GPUResource.h"

// GPUResource Interitance structure.                                                           Graphicaly
//                                                                                                  ^
//    GPUResource                                                           Programmatically    <式式式托式式式> User-Friendly 
//         弛                                                                                       
//       忙式扛式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式忖
//    UAVBuffer   LinearAllocationPage         PixelBuffer          Texture
//        弛                                         弛
//        弛                                         戍式式式式式式式式式式式式式式式式式式式忖
//        弛                                    DepthBuffer         ColorBuffer
//        弛                                         弛
//        弛                                    ShadowBuffer
//        戍式式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式忖
// ByteAddressBuffer        ReadyBackBuffer       TypedBuffer       StructuredBuffer   TypedBuffer
//        弛
// DispatchIndirectBuffer

namespace custom
{
	class UAVBuffer : public GPUResource
	{
		friend class ComputeContext;
	public:
		virtual ~UAVBuffer()
		{
			Destroy();
		}

		// Create a buffer. 
		// If initial data is provided, it will be copied into the buffer using the default command context.
		void Create
		(
			const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
			const void* initialData = nullptr
		);

		// Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
		void CreatePlaced
		(
			const std::wstring& name, ID3D12Heap* pBackingHeap, 
			uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
			const void* initialData = nullptr
		);

		D3D12_GPU_VIRTUAL_ADDRESS RootConstantBufferView(void) const 
		{ 
			return m_GPUVirtualAddress; 
		}

		D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView(uint32_t Offset, uint32_t Size) const;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
		{
			D3D12_VERTEX_BUFFER_VIEW VBView;
			VBView.BufferLocation = m_GPUVirtualAddress + Offset;
			VBView.SizeInBytes = Size;
			VBView.StrideInBytes = Stride;
			return VBView;
		}
		D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t BaseVertexIndex = 0) const
		{
			size_t Offset = BaseVertexIndex * m_elementSize;
			return VertexBufferView(Offset, (uint32_t)(m_bufferSize - Offset), m_elementSize);
		}

		D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const
		{
			D3D12_INDEX_BUFFER_VIEW IBView;
			IBView.BufferLocation = m_GPUVirtualAddress + Offset;
			IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			IBView.SizeInBytes = Size;
			return IBView;
		}
		D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t StartIndex = 0) const
		{
			size_t Offset = StartIndex * m_elementSize;
			return IndexBufferView(Offset, (uint32_t)(m_bufferSize - Offset), m_elementSize == 4);
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const
		{
			return m_UAV;
		}
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const
		{
			return m_SRV;
		}
		size_t GetBufferSize() const 
		{ 
			return m_bufferSize; 
		}
		uint32_t GetElementCount() const 
		{ 
			return m_elementCount; 
		}
		uint32_t GetElementSize() const 
		{ 
			return m_elementSize; 
		}

	protected:
		UAVBuffer()
			: m_bufferSize(0), m_elementCount(0), m_elementSize(0)
		{
			m_resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		virtual void CreateUAV(std::wstring Name) = 0;
		D3D12_RESOURCE_DESC resourceDescriptor();

	protected:
		D3D12_CPU_DESCRIPTOR_HANDLE m_UAV;
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRV;

		size_t   m_bufferSize;
		uint32_t m_elementCount;
		uint32_t m_elementSize;
		D3D12_RESOURCE_FLAGS m_resourceFlags;
	};
}

namespace custom
{
	class ByteAddressBuffer : public UAVBuffer
	{
	public:
		~ByteAddressBuffer()
		{
			Destroy();
		}
		virtual void CreateUAV(std::wstring Name = L"") override;

		static ByteAddressBuffer* CreateIndexBuffer
		(
			const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
			const void* initialData
		);
		virtual void Destroy() override
		{
			UAVBuffer::Destroy();
			GPUResource::Destroy();
		}
		static void DestroyIndexAllBuffer();
	};

	class StructuredBuffer : public UAVBuffer
	{
	public:
		~StructuredBuffer()
		{
			Destroy();
		}
		virtual void CreateUAV(std::wstring Name = L"") override;

		static StructuredBuffer* CreateVertexBuffer
		(
			const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
			const void* initialData
		);

		static void DestroyAllVertexBuffer();

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterSRV(CommandContext& Context);
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterUAV(CommandContext& Context);

		ByteAddressBuffer& GetCounterBuffer() 
		{ 
			return m_CounterBuffer; 
		}

		virtual void Destroy() override
		{
			m_CounterBuffer.Destroy();
			UAVBuffer::Destroy();
			GPUResource::Destroy();
		}
	private:
		ByteAddressBuffer m_CounterBuffer;
	};

	// // For Dispatch Indirect
	class IndirectArgsBuffer : public ByteAddressBuffer
	{
	public:
		IndirectArgsBuffer()
		{
		}
	};

	class TypedBuffer : public UAVBuffer
	{
	public:
		TypedBuffer(DXGI_FORMAT Format) 
			: m_DataFormat(Format) 
		{
		}
		virtual void CreateUAV(std::wstring Name = L"") override;

	protected:
		DXGI_FORMAT m_DataFormat;
	};

}