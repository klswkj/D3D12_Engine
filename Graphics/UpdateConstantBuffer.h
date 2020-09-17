#pragma once
#include "stdafx.h"
#include "RawBuffer.h"
#include "device.h"
#include "RenderingResource.h"
#include "CommandContext.h"

#include "DynamicUploadBuffer.h"

class UpdateConstantBuffer : public RenderingResource
{
public:
	void Update(const RawBuffer& buf)
	{
		ASSERT(&buf.GetRootLayoutElement() == &GetRootLayoutElement());
		
		const UINT kBufferSize = buf.GetSizeInBytes() * sizeof(char);

		uint8_t* pDataBegin;
		ASSERT_HR(pConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pDataBegin)));
		memcpy(pDataBegin, buf.GetData(), kBufferSize);
		pConstantBuffer->Unmap(0, nullptr);
	}

	virtual const IPolymorphData& GetRootLayoutElement() const noexcept = 0;
protected:
	UpdateConstantBuffer(const IPolymorphData& layoutRoot, UINT slot, const RawBuffer* pBuf)
		: slot(slot)
	{
		D3D12_HEAP_PROPERTIES HeapProps;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;
		HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC ResourceDesc;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ResourceDesc.Alignment = 0;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ResourceDesc.Width = (UINT)layoutRoot.GetSizeInBytes();
		ResourceDesc.Height = 1;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;

		if (pBuf != nullptr) // pBuf가 있으면, pBuf의 데이터를 복사해서, D3D12Resource 만듬.
		{
			ASSERT_HR(device::g_pDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&ResourceDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&pConstantBuffer)));

			const UINT kBufferSize = pBuf->GetSizeInBytes() * sizeof(char);

			uint8_t* pDataBegin;
			ASSERT_HR(pConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pDataBegin)));
			memcpy(pDataBegin, pBuf->GetData(), kBufferSize);
			pConstantBuffer->Unmap(0, nullptr);
		}
		else // 없으면 그냥 만듬.
		{
			ASSERT_HR(device::g_pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pConstantBuffer)));
		}
	}
protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> pConstantBuffer;
	UINT slot;
};

class UpdatePixelConstantBuffer : public UpdateConstantBuffer
{
public:
	using UpdateConstantBuffer::UpdateConstantBuffer;
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{
		// RootSiganture
		// GetContext(gfx)->PSSetConstantBuffers(slot, 1u, pConstantBuffer.GetAddressOf());
	}
};

class UpdateVertexConstantBuffer : public UpdateConstantBuffer
{
public:
	using UpdateConstantBuffer::UpdateConstantBuffer;
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{
		// RootSignature
		// GetContext(gfx)->VSSetConstantBuffers(slot, 1u, pConstantBuffer.GetAddressOf()); 
	}
};

class CachingPixelConstantBufferEx : public UpdatePixelConstantBuffer
{
public:
	CachingPixelConstantBufferEx(const ManagedLayout& layout, UINT slot)
		: UpdatePixelConstantBuffer(*layout.ShareRoot(), slot, nullptr), rawBuffer(RawBuffer(layout))
	{
	}
	CachingPixelConstantBufferEx(const RawBuffer& _RawBuffer, UINT slot)
		: UpdatePixelConstantBuffer(_RawBuffer.GetRootLayoutElement(), slot, &rawBuffer), rawBuffer(_RawBuffer)
	{
	}
	const IPolymorphData& GetRootLayoutElement() const noexcept override
	{
		return rawBuffer.GetRootLayoutElement();
	}
	const RawBuffer& GetBuffer() const noexcept
	{
		return rawBuffer;
	}
	void SetBuffer(const RawBuffer& _RawBuffer)
	{
		rawBuffer.CopyFrom(_RawBuffer);
		bDirty = true;
	}
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT override
	{
		if (bDirty)
		{
			UpdatePixelConstantBuffer::Update(rawBuffer);
			bDirty = false;
		}
		UpdatePixelConstantBuffer::Bind(BaseContext);
	}

private:
	bool bDirty = false;
	RawBuffer rawBuffer;
};