#pragma once
#include "RootParameter.h"
#include "RenderingResource.h"

// Maximum 64 DWORDS divied up amongst all root parameters.
// Root constants = 1 DWORD * NumConstants
// Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
// Descriptor table pointer = 1 DWORD
// Static samplers = 0 DWORDS (compiled into shader)

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
		}

		~RootSignature()
		{
			// delete[] m_rootParamArray;
			// delete[] m_staticSamplerArray;
			/*
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
			*/
		}

		void Reset(uint32_t NumRootParams, uint32_t NumStaticSamplers = 0)
		{
			if (0 < NumRootParams)
			{
				m_rootParamArray.reset(new RootParameter[NumRootParams]);

				// delete[] m_rootParamArray;
				//m_rootParamArray = new RootParameter[NumRootParams];
			}
			else
			{
				m_rootParamArray = nullptr;
			}


			if (0 < NumStaticSamplers)
			{
				m_staticSamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);

				// delete[] m_staticSamplerArray;
				// m_staticSamplerArray = new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers];
			}
			else
			{
				m_staticSamplerArray = nullptr;
			}

			m_numRootParameters = NumRootParams;
			m_numStaticSamplers = NumStaticSamplers;
			m_numInitializedStaticSamplers = 0;
		}

		RootParameter& operator[] (size_t EntryIndex)
		{
			ASSERT(EntryIndex < m_numRootParameters);
			// return m_rootParamArray.get()[EntryIndex];
			return m_rootParamArray.get()[EntryIndex];
		}

		const RootParameter& operator[] (size_t EntryIndex) const
		{
			ASSERT(EntryIndex < m_numRootParameters);
			// return m_rootParamArray.get()[EntryIndex];
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

		static void DestroyAll();

		void Bind(CommandContext& BaseContext) DEBUG_EXCEPT override;

	protected:
		std::shared_ptr<RootParameter[]> m_rootParamArray;
		std::shared_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_staticSamplerArray;

		// RootParameter* m_rootParamArray;
		// D3D12_STATIC_SAMPLER_DESC* m_staticSamplerArray;

		ID3D12RootSignature* m_rootSignature;

		BOOL     m_finalized;
		BOOL     m_firstCompiled = false;
		uint32_t m_numRootParameters;
		uint32_t m_numStaticSamplers;
		uint32_t m_numInitializedStaticSamplers;
		uint32_t m_descriptorTableBitMap;         // One bit is set for root parameters that are non-sampler descriptor tables
		uint32_t m_staticSamplerTableBitMap;      // One bit is set for root parameters that are sampler descriptor tables
		uint32_t m_descriptorTableSize[16];       // Non-sampler descriptor tables need to know their descriptor count
	};
}

class RootSignatureManager
{

};