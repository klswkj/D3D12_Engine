#pragma once
#include "RootParameter.h"
#include "RenderingResource.h"

// Maximum 64 DWORDS divied up amongst all root parameters.
// Root constants = 1 DWORD * NumConstants
// Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
// Descriptor table pointer = 1 DWORD
// Static samplers = 0 DWORDS (compiled into shader)

#define DESCRIPTOR_TABLE_SIZE 16

#define _SHARED_PTR 1
// #define _RAW_PTR 0

namespace custom
{
	class CommandContext;

	class RootSignature : public RenderingResource
	{
		friend class DynamicDescriptorHeap;
	public:
		RootSignature(UINT NumRootParams = 0, UINT NumStaticSamplers = 0)
			: m_finalized(FALSE), m_numRootParameters(NumRootParams)
		{
			Reset(NumRootParams, NumStaticSamplers);
			m_firstCompiled = false;
			m_rootSignature = nullptr;
		}

		void operator=(const RootSignature& Other)
		{
			ASSERT(Other.m_numInitializedStaticSamplers == Other.m_numStaticSamplers);

			Reset(Other.m_numRootParameters, Other.m_numStaticSamplers);
			m_numInitializedStaticSamplers = Other.m_numInitializedStaticSamplers;
#ifdef _RAW_PTR
			memcpy_s(&m_rootParamArray[0], sizeof(RootParameter) * m_numRootParameters, &Other.m_rootParamArray[0], sizeof(RootParameter) * m_numRootParameters);
			memcpy_s(&m_staticSamplerArray[0], sizeof(D3D12_STATIC_SAMPLER_DESC) * m_numStaticSamplers, &Other.m_staticSamplerArray[0], sizeof(D3D12_STATIC_SAMPLER_DESC) * m_numStaticSamplers);
#else
			memcpy(&m_rootParamArray.get()[0], &Other.m_rootParamArray.get()[0], sizeof(RootParameter) * m_numRootParameters);
			memcpy(&m_staticSamplerArray.get()[0], &Other.m_staticSamplerArray.get()[0], sizeof(D3D12_STATIC_SAMPLER_DESC) * m_numStaticSamplers);
#endif

			for (size_t i = 0; i < m_numRootParameters; ++i)
			{
				if (Other.m_rootParamArray.get()[i].m_D3D12RootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
				{
					UINT RangeCount = m_rootParamArray.get()[i].m_D3D12RootParameter.DescriptorTable.NumDescriptorRanges;

					m_rootParamArray.get()[i].m_D3D12RootParameter.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];

					memcpy_s
					(
						(void*)&m_rootParamArray.get()[i].m_D3D12RootParameter.DescriptorTable.pDescriptorRanges[0],
						RangeCount * sizeof(D3D12_DESCRIPTOR_RANGE),
						&Other.m_rootParamArray.get()[i].m_D3D12RootParameter.DescriptorTable.pDescriptorRanges[0],
						RangeCount * sizeof(D3D12_DESCRIPTOR_RANGE)
					);
				}
			}

			for (size_t i = 0; i < DESCRIPTOR_TABLE_SIZE; ++i)
			{
				m_descriptorTableSize[i] = Other.m_descriptorTableSize[i];
			}

			m_finalized = Other.m_finalized;

			if (m_finalized)
			{
				m_rootSignature = Other.m_rootSignature;
			}

			m_firstCompiled            = Other.m_firstCompiled;
			m_descriptorTableBitMap    = Other.m_descriptorTableBitMap;
			m_staticSamplerTableBitMap = Other.m_staticSamplerTableBitMap;
		}

#ifdef _RAW_PTR
		~RootSignature()
		{
			if (m_firstCompiled)
			{
				if (m_rootParamArray != nullptr)
				{
					delete[] m_rootParamArray;
					m_rootParamArray = nullptr;
				}

				if (m_staticSamplerArray != nullptr)
				{
					delete[] m_staticSamplerArray;
					m_staticSamplerArray = nullptr;
				}
			}
		}
#endif

		void Reset(uint32_t NumRootParams, uint32_t NumStaticSamplers = 0)
		{
			if (0 < NumRootParams)
			{
#ifdef _RAW_PTR
				delete[] m_rootParamArray;
				m_rootParamArray = new RootParameter[NumRootParams];
#else
				m_rootParamArray.reset(new RootParameter[NumRootParams]);
#endif
			}
			else
			{
#ifdef _RAW_PTR
				if (m_rootParamArray)
				{
					delete[] m_rootParamArray;
				}
#endif
				m_rootParamArray = nullptr;
			}


			if (0 < NumStaticSamplers)
			{
#ifdef _RAW_PTR
				delete[] m_staticSamplerArray;
				m_staticSamplerArray = new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers];
#else
				m_staticSamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
#endif
			}
			else
			{
#ifdef _RAW_PTR
				if (m_staticSamplerArray)
				{
					delete[] m_rootParamArray;
				}
#endif
				m_staticSamplerArray = nullptr;
			}

			m_numRootParameters = NumRootParams;
			m_numStaticSamplers = NumStaticSamplers;
			m_numInitializedStaticSamplers = 0;
		}

		RootParameter& operator[] (size_t EntryIndex)
		{
			ASSERT(EntryIndex < m_numRootParameters);
#ifdef _RAW_PTR
			return m_rootParamArray[EntryIndex];
#else
			return m_rootParamArray.get()[EntryIndex];
#endif
		}

		const RootParameter& operator[] (size_t EntryIndex) const
		{
			ASSERT(EntryIndex < m_numRootParameters);
#ifdef _RAW_PTR
			return m_rootParamArray[EntryIndex];
#else
		    return m_rootParamArray.get()[EntryIndex];
#endif
		}

		void InitStaticSampler
		(
			uint32_t Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
			D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL
			);

		void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

		ID3D12RootSignature* GetSignature() const 
		{ 
			ASSERT(m_rootSignature != nullptr);
			return m_rootSignature; 
		}

		static void DestroyAll();

		void Bind(CommandContext& BaseContext) DEBUG_EXCEPT override;
	public:
		BOOL     m_finalized;
		uint32_t m_numRootParameters;
		uint32_t m_numStaticSamplers;
		uint32_t m_numInitializedStaticSamplers;

	protected:
#ifdef _RAW_PTR
		RootParameter* m_rootParamArray = nullptr;
		D3D12_STATIC_SAMPLER_DESC* m_staticSamplerArray = nullptr;
#else
		std::shared_ptr<RootParameter[]>             m_rootParamArray;
		std::shared_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_staticSamplerArray;
#endif
		ID3D12RootSignature* m_rootSignature;

		BOOL     m_firstCompiled;
		uint32_t m_descriptorTableBitMap;                      // One bit is set for root parameters that are non-sampler descriptor tables
		uint32_t m_staticSamplerTableBitMap;                   // One bit is set for root parameters that are sampler descriptor tables
		uint32_t m_descriptorTableSize[DESCRIPTOR_TABLE_SIZE]; // Non-sampler descriptor tables need to know their descriptor count
	};
}

class RootSignatureManager
{

};