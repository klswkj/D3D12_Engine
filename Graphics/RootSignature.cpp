#include "stdafx.h"
#include "RootSignature.h"
#include "Device.h"
// #include "CommandContext.h"
#include "ComputeContext.h"

static std::map<size_t, ID3D12RootSignature*> s_RootSignatureMap;

namespace custom
{
    void RootSignature::DestroyAll()
    {
        for (auto& e : s_RootSignatureMap)
        {
            e.second->Release();
            e.second = nullptr;
        }

        s_RootSignatureMap.clear();
    }

    void RootSignature::Bind(CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
    {
        ASSERT(m_finalized, "This RootSignature is not finalized yet");

        custom::GraphicsContext* graphicsContext = nullptr;
        custom::ComputeContext* computeContext = nullptr;

        if (graphicsContext = dynamic_cast<custom::GraphicsContext*>(&BaseContext))
        {
            graphicsContext->SetRootSignature(*this, commandIndex);
        }
        else if(computeContext = dynamic_cast<custom::ComputeContext*>(&BaseContext))
        {
            computeContext->SetRootSignature(*this, commandIndex);
        }
    }

    void RootSignature::InitStaticSampler
    (
        const uint32_t Register,
        const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
        const D3D12_SHADER_VISIBILITY Visibility /* = D3D12_SHADER_VISIBILITY_ALL */
    )
    {
        ASSERT(m_numInitializedStaticSamplers < m_numStaticSamplers);
        D3D12_STATIC_SAMPLER_DESC& StaticSamplerDesc = m_staticSamplerArray[m_numInitializedStaticSamplers++];

        StaticSamplerDesc.Filter = NonStaticSamplerDesc.Filter;
        StaticSamplerDesc.AddressU = NonStaticSamplerDesc.AddressU;
        StaticSamplerDesc.AddressV = NonStaticSamplerDesc.AddressV;
        StaticSamplerDesc.AddressW = NonStaticSamplerDesc.AddressW;
        StaticSamplerDesc.MipLODBias = NonStaticSamplerDesc.MipLODBias;
        StaticSamplerDesc.MaxAnisotropy = NonStaticSamplerDesc.MaxAnisotropy;
        StaticSamplerDesc.ComparisonFunc = NonStaticSamplerDesc.ComparisonFunc;
        StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        StaticSamplerDesc.MinLOD = NonStaticSamplerDesc.MinLOD;
        StaticSamplerDesc.MaxLOD = NonStaticSamplerDesc.MaxLOD;
        StaticSamplerDesc.ShaderRegister = Register;
        StaticSamplerDesc.RegisterSpace = 0;
        StaticSamplerDesc.ShaderVisibility = Visibility;

        if (StaticSamplerDesc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
            StaticSamplerDesc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
            StaticSamplerDesc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
        {
            ASSERT
            (
                // Transparent Black
                NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
                NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
                NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
                NonStaticSamplerDesc.BorderColor[3] == 0.0f ||
                // Opaque Black
                NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
                NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
                NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
                NonStaticSamplerDesc.BorderColor[3] == 1.0f ||
                // Opaque White
                NonStaticSamplerDesc.BorderColor[0] == 1.0f &&
                NonStaticSamplerDesc.BorderColor[1] == 1.0f &&
                NonStaticSamplerDesc.BorderColor[2] == 1.0f &&
                NonStaticSamplerDesc.BorderColor[3] == 1.0f,
                "Sampler border color does not match static sampler limitations"
            );

            if (NonStaticSamplerDesc.BorderColor[3] == 1.0f)
            {
                if (NonStaticSamplerDesc.BorderColor[0] == 1.0f)
                {
                    StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
                }
                else
                {
                    StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                }
            }
            else
            {
                StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            }
        }
    }

    void RootSignature::Finalize(const std::wstring& name, const D3D12_ROOT_SIGNATURE_FLAGS Flags /*= D3D12_ROOT_SIGNATURE_FLAG_NONE*/)
    {
        ASSERT(m_finalized == false);
        ASSERT(m_numInitializedStaticSamplers == m_numStaticSamplers);
        ASSERT(2 <= name.size());

        D3D12_ROOT_SIGNATURE_DESC RootDesc;
        RootDesc.NumParameters = m_numRootParameters;
#ifdef _RAW_PTR
        RootDesc.pParameters = (const D3D12_ROOT_PARAMETER*)m_rootParamArray;
#else
        RootDesc.pParameters = (const D3D12_ROOT_PARAMETER*)m_rootParamArray.get();
#endif

        RootDesc.NumStaticSamplers = m_numStaticSamplers;
#ifdef _RAW_PTR
        RootDesc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC*)m_staticSamplerArray;
#else
        RootDesc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC*)m_staticSamplerArray.get();
#endif
        RootDesc.Flags = Flags;

#undef RAW_PTR
#undef SHARED_PTR

        m_descriptorTableBitMap = 0;
        m_staticSamplerTableBitMap = 0;

        size_t HashCode = Hash::MakeHash(&RootDesc.Flags);
        HashCode        = Hash::MakeHash(RootDesc.pStaticSamplers, m_numStaticSamplers, HashCode);

        for (size_t IParam = 0; IParam < m_numRootParameters; ++IParam)
        {
            const D3D12_ROOT_PARAMETER& RootParam = RootDesc.pParameters[IParam];
            m_descriptorTableSize[IParam] = 0;

            if (RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            {
                ASSERT(RootParam.DescriptorTable.pDescriptorRanges != nullptr);

                HashCode = Hash::MakeHash
                (
                    RootParam.DescriptorTable.pDescriptorRanges,
                    RootParam.DescriptorTable.NumDescriptorRanges,
                    HashCode
                );

                // We keep track of sampler descriptor tables separately from CBV_SRV_UAV descriptor tables
                if (RootParam.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
                {
                    m_staticSamplerTableBitMap |= (1 << IParam);
                }
                else
                {
                    m_descriptorTableBitMap |= (1 << IParam);
                }
                for (uint32_t ITableRange = 0; ITableRange < RootParam.DescriptorTable.NumDescriptorRanges; ++ITableRange)
                {
                    m_descriptorTableSize[IParam] += RootParam.DescriptorTable.pDescriptorRanges[ITableRange].NumDescriptors;
                }
            }
            {
                HashCode = Hash::MakeHash(&RootParam, 1, HashCode);
            }
        }

        ID3D12RootSignature** RSRef = nullptr;
        // bool firstCompiled = false;
        {
            static std::mutex s_HashMapMutex;
            std::lock_guard<std::mutex> CS(s_HashMapMutex);

            auto iter = s_RootSignatureMap.find(HashCode);

            // Reserve space so the next inquiry will find that someone got here first.
            if (iter == s_RootSignatureMap.end())
            {
                RSRef = &s_RootSignatureMap[HashCode];
                m_firstCompiled = true;
            }
            else
            {
                RSRef = &iter->second;
            }
        }

		if (m_firstCompiled)
		{
			Microsoft::WRL::ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

            HRESULT hardwareResult;

			ASSERT_HR
			(
                hardwareResult = D3D12SerializeRootSignature
				(
					&RootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
					pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf()
				)
			);

			ASSERT_HR
			(
                hardwareResult = device::g_pDevice->CreateRootSignature
				(
					1, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)
				)
			);
#if defined(_DEBUG)
            m_rootSignature->SetName(name.c_str());
#endif
            s_RootSignatureMap[HashCode] = m_rootSignature;

            ASSERT(*RSRef == m_rootSignature);
		}
		else
		{
			while (*RSRef == nullptr)
			{
				std::this_thread::yield();
			}
            m_rootSignature = *RSRef;
		}

        m_finalized = true;
    }
}