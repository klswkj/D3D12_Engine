#pragma once
#include "RootParameter.h"
// Maximum 64 DWORDS divied up amongst all root parameters.
// Root constants = 1 DWORD * NumConstants
// Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
// Descriptor table pointer = 1 DWORD
// Static samplers = 0 DWORDS (compiled into shader)


namespace custom
{
	class RootSignature
	{
		friend class DynamicDescriptorHeap;

	public:
		RootSignature(uint32_t NumRootParams = 0, uint32_t NumStaticSamplers = 0)
			: m_finalized(FALSE), 
			  m_numRootParameters(NumRootParams)
		{
			Reset(NumRootParams, NumStaticSamplers);
		}

		~RootSignature()
		{
		}

		void Reset(uint32_t NumRootParams, uint32_t NumStaticSamplers = 0)
		{
			if (0 < NumRootParams)
			{
				m_rootParamArray.reset(new RootParameter[NumRootParams]);
			}
			else
			{
				m_rootParamArray = nullptr;
			}

			m_numRootParameters = NumRootParams;

			if (0 < NumStaticSamplers)
			{
				m_staticSamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
			}
			else
			{
				m_staticSamplerArray = nullptr;
			}
			/*
			static uint32_t Index = -1;

			m_hashKey = typeid(this).name() + '#' + std::to_string(++Index);
			
			if (pContainer->find(m_hashKey) == pContainer->end())
			{
				pContainer->emplace(m_hashKey, this);
			}
			*/
			m_numStaticSamplers = NumStaticSamplers;
			m_numInitializedStaticSamplers = 0;
		}

		custom::RootParameter& operator[] (size_t EntryIndex)
		{
			ASSERT(EntryIndex < m_numRootParameters);
			return m_rootParamArray.get()[EntryIndex];
		}

		const custom::RootParameter& operator[] (size_t EntryIndex) const
		{
			ASSERT(EntryIndex < m_numRootParameters);
			return m_rootParamArray.get()[EntryIndex];
		}

		void InitStaticSampler
		(
			uint32_t Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
			D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL
			);

		void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

		ID3D12RootSignature* GetSignature() const 
		{ 
			return m_rootSignature; 
		}

	protected:
		std::unique_ptr<custom::RootParameter[]>             m_rootParamArray;
		std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_staticSamplerArray;

		ID3D12RootSignature*                         m_rootSignature;

		// Rootsignature is re-used in D3D12Device.
		// CONTAINED RootSignatureManager*              m_pContainer;

		BOOL     m_finalized;
		uint32_t m_numRootParameters;
		uint32_t m_numStaticSamplers;
		uint32_t m_numInitializedStaticSamplers;
		uint32_t m_descriptorTableBitMap;         // One bit is set for root parameters that are non-sampler descriptor tables
		uint32_t m_staticSamplerTableBitMap;            // One bit is set for root parameters that are sampler descriptor tables
		uint32_t m_descriptorTableSize[16];       // Non-sampler descriptor tables need to know their descriptor count
	};
}