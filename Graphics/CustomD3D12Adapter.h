#pragma once

// Ring 0  - Global
// Ring .5 -
// Ring 1  - D3D12GPUAdapter,
// Ring 2  - D3D12GPUNode, FrameFenceManager, DeletionQueue    RootSignatureManager
// Ring 3  - ResidencyManager, CommandQueueManager, CommandContextManager


// Another Ring 1 - TextureManager, BufferManager, CameraManager(ก่?), AccelerationStructureManager(ก่?), EntityManager,

#define MIN_PLACED_BUFFER_SIZE _KB(16u)
#define D3D_BUFFER_ALIGNMENT   _KB(16u)

#define DEFAULT_BUFFER_POOL_SIZE _MB(16u)

#define DEFAULT_BUFFER_POOL_MAX_ALLOC_SIZE     _MB(64u)
#define DEFAULT_BUFFER_POOL_DEFAULT_POOL_SIZE  _MB(16u)

#define READBACK_BUFFER_POOL_MAX_ALLOC_SIZE    _KB(64u)
#define READBACK_BUFFER_POOL_DEFAULT_POOL_SIZE _MB(4u)

#define DEFAULT_CONTEXT_UPLOAD_POOL_SIZE           _MB(8u)
#define DEFAULT_CONTEXT_UPLOAD_POOL_MAX_ALLOC_SIZE _MB(4u)
#define DEFAULT_CONTEXT_UPLOAD_POOL_ALIGNMENT (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)

#define TEXTURE_POOL_SIZE _MB(8)

namespace custom
{
	class D3D12GPUNode;

	// TODO 0 : At Device.cpp to Array of custom::D3D12Adapter. 
	class D3D12Adapter
	{
	public:
		explicit D3D12Adapter
		(
			IDXGIFactory4* pDXGIFactory,
			uint32_t       adapterIndexInPhysicalMachine
		)
			:
			m_pParentFactory(pDXGIFactory),
			m_pDXGIAdapter3(nullptr),
			m_AdapterIndexInPhysicalMachine(adapterIndexInPhysicalMachine),
			m_pRootDevice3(nullptr),
			m_pDXRDevice5(nullptr),
			m_Desc({}),
			m_MaxSupportedFeatureLevel(D3D_FEATURE_LEVEL_1_0_CORE),
			m_ResourceHeapTier(D3D12_RESOURCE_HEAP_TIER_1),
			m_ResourceBindingTier(D3D12_RESOURCE_BINDING_TIER_1),
			m_RootSignatureVersion(D3D_ROOT_SIGNATURE_VERSION_1),
			m_pFrameFence(nullptr),
			m_FrameFenceValue(0ull)
		{
			ASSERT(m_pParentFactory);
			::ZeroMemory(&m_D3D12GPUNodes, sizeof(custom::D3D12GPUNode*) * _countof(m_D3D12GPUNodes)); // Raw Array ver.
			// TODO 0 : Create D3D12Devices and Spec(Specific? I Don't remeber...) Descriptor.
		}

		~D3D12Adapter()
		{
			DestoryD3D12Adapter();
		}

		HRESULT InitializeD3D12Adapter();
		void DestoryD3D12Adapter();

		inline bool GetSupportedDXR() const { return (bool)m_pDXRDevice5; };
		::DXGI_QUERY_VIDEO_MEMORY_INFO GetLocalVideoMemoryInfo() const;
		inline ::IDXGIAdapter3* const GetRawAdapter() const { return m_pDXGIAdapter3; }
		inline ::ID3D12Device* const GetRawDevice() const { return m_pRootDevice3; }
		inline uint64_t GetFrameFenceValue() const
		{
			return InterlockedGetValue(&m_FrameFenceValue);
		}
		inline bool IsFrameFenceComplete(uint64_t fenceValue) const
		{
			uint64_t LastFrameFenceValue = GetFrameFenceValue();

			if (LastFrameFenceValue <= fenceValue)
			{
				uint64_t GPUSideLastCompletedFrameFenceValue = m_pFrameFence->GetCompletedValue();

				InterlockedExchange64
				(
					(volatile LONG64*)m_FrameFenceValue,
					(GPUSideLastCompletedFrameFenceValue <= LastFrameFenceValue)
					? LastFrameFenceValue : GPUSideLastCompletedFrameFenceValue
				);
			}

			return fenceValue <= GetFrameFenceValue();
		}
		inline uint32_t GetPhysicalAdapterIndex() const
		{
			return m_AdapterIndexInPhysicalMachine;
		}
		void EnqueueD3D12Object(ID3D12Object* pObject, uint64_t fenceValue);
		void ReleaseD3D12Object(uint64_t fenceValue);

	protected:
		IDXGIFactory4* m_pParentFactory;
		IDXGIAdapter3* m_pDXGIAdapter3;
		ID3D12Device3* m_pRootDevice3; // For Advanced manage IPagableObject Residency
		ID3D12Device5* m_pDXRDevice5;

		/** The maximum D3D12 feature level supported. 0 if not supported or FindAdpater() wasn't called */
		D3D_FEATURE_LEVEL           m_MaxSupportedFeatureLevel;
		DXGI_ADAPTER_DESC           m_Desc;
		D3D12_RESOURCE_HEAP_TIER    m_ResourceHeapTier;
		D3D12_RESOURCE_BINDING_TIER m_ResourceBindingTier;
		D3D_ROOT_SIGNATURE_VERSION  m_RootSignatureVersion;

		uint32_t m_AdapterIndexInPhysicalMachine;
		// https://www.programmersought.com/article/1850548948/
		// std::vector<custom::D3D12GPUNode*> m_D3D12GPUNodes;
		custom::D3D12GPUNode* m_D3D12GPUNodes[4];

		::ID3D12Fence*            m_pFrameFence;
		mutable volatile uint64_t m_FrameFenceValue;

		std::queue<std::pair<ID3D12Object*, uint64_t>> m_DeferredDeletionQueue;
		// D3D12_COMMAND_LIST_TYPE m_FenceType;
	};

	// TODO 0 : Devices will create Allocators.
}