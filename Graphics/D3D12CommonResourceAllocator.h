#pragma once

namespace custom
{
	class D3D12GPUNode;
	class D3D12ResourceLocation;
}

enum class eAllocatorType;
class D3D12MultiBuddyAllocator;

// It Will be moved to CustomD3D12Adapter.h
enum class eBufferUsageFlags
{
	BufferUsageFlags_None                  = 0x0000,
	BufferUsageFlags_Static                = 0x0001,
	BufferUsageFlags_Dynamic               = 0x0002,
	BufferUsageFlags_Volatile              = 0x0004,
	BufferUsageFlags_UnorderedAccess       = 0x0008,
	BufferUsageFlags_ByteAddressBuffer     = 0x0010,
	BufferUsageFlags_SourceCopy            = 0x0020,
	BufferUsageFlags_StreamOutput          = 0x0040,
	BufferUsageFlags_DrawIndirect          = 0x0080,
	BufferUsageFlags_ShaderResource        = 0x0100,
	BufferUsageFlags_KeepCPUAccessible     = 0x0200,
	BufferUsageFlags_ZeroStride            = 0x0400,
	BufferUsageFlags_FastVRAM              = 0x0800,
	BufferUsageFlags_Transient             = 0x1000,
	BufferUsageFlags_Shared                = 0x2000,
	BufferUsageFlags_AccelerationStructure = 0x4000,
	BufferUsageFlags_VertexBuffer          = 0x8000,
	BufferUsageFlags_IndexBuffer           = 0x10000,
	BufferUsageFlags_StructuredBuffer      = 0x20000,
	BufferUsageFlags_AnyDynamic            = (BufferUsageFlags_Dynamic | BufferUsageFlags_Volatile), // Go to DynamicAllocator
};
DEFINE_ENUM_FLAG_OPERATORS(eBufferUsageFlags);

// MultiBuddyAllocator
// Vertex, Index, Structured, Hight level Accelrated Buffer(DXR)
class D3D12CommonResourceAllocator
{
public:
	enum class eViewType
	{
		None = 0, // If Allow D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE
		SRV,
		UAV,      // If Allow UNORDRED_ACCESS
		Count
	};
	
	explicit D3D12CommonResourceAllocator(custom::D3D12GPUNode* const pParentGPUNode, const D3D12_NODE_MASK visibleNdoeMask);
	~D3D12CommonResourceAllocator();

	void InitializeAllocator(const D3D12CommonResourceAllocator::eViewType ePoolViewType, const D3D12_RESOURCE_FLAGS resourceFlags);
	void CleanUpMultiAllocator(D3D12CommonResourceAllocator::eViewType multiAllocatorType, size_t fenceValue);

	bool AllocCommonResource
	(
		const D3D12_RESOURCE_DESC& resourceDesc, 
		custom::D3D12ResourceLocation& resourceLocation, 
		const eBufferUsageFlags eBufferUsageFlag,  
		const uint32_t align
	);
	void FreeDefaultBufferPools(uint64_t fenceValue);

	void SetFenceValue(uint64_t fenceValue)
	{
		ASSERT(m_LastFenceValue <= fenceValue);
		m_LastFenceValue = fenceValue;
	}

private:
	inline D3D12CommonResourceAllocator::eViewType GetBufferPool(const D3D12_RESOURCE_FLAGS resourceFlags) const
	{
		if (resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		{
			return eViewType::UAV;
		}
		else if (resourceFlags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)
		{
			return eViewType::None;
		}
		else
		{
			return eViewType::SRV;
		}
	}
	inline bool BufferIsWriteable(const D3D12_RESOURCE_DESC& resourceDesc)
	{
		const bool bDSV = (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)    != 0;
		const bool bRTV = (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)    != 0;
		const bool bUAV = (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;

		// Buffer Depth Stencils are invalid
		ASSERT(!bDSV);

		const bool bWriteable = bDSV || bRTV || bUAV;

		return bWriteable;
	}

private:
	custom::D3D12GPUNode*     m_pParentGPUNode;
	D3D12_NODE_MASK           m_VisibleNodeMask;
	uint64_t                  m_LastFenceValue;
	D3D12MultiBuddyAllocator* m_pMultiBuddyAllocator[(size_t)eViewType::Count];
};