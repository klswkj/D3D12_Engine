#pragma once
#include "stdafx.h"
#include "Device.h"
#include "IndirectParameter.h"
#include "RootSignature.h"

namespace custom
{
	class CommandSignature
	{
    public:
        CommandSignature(UINT NumParams = 0) 
            : m_bFinalized(FALSE), m_numParameters(NumParams)
        {
            Reset(NumParams);
        }

        void Destroy(void)
        {
            SafeRelease(m_pCommandSignature);
            m_pIndirectParameters = nullptr;
        }

        void Reset(UINT NumParams)
        {
			if (0 < NumParams)
			{
				m_pIndirectParameters.reset(new IndirectParameter[NumParams]);
			}
			else
			{
				m_pIndirectParameters = nullptr;
			}

            m_numParameters = NumParams;
        }

        IndirectParameter& operator[] (size_t EntryIndex)
        {
            ASSERT(EntryIndex < m_numParameters);
            return m_pIndirectParameters.get()[EntryIndex];
        }

        const IndirectParameter& operator[] (size_t EntryIndex) const
        {
            ASSERT(EntryIndex < m_numParameters);
            return m_pIndirectParameters.get()[EntryIndex];
        }

        void Finalize(const RootSignature* RootSignature = nullptr);

        ID3D12CommandSignature* GetSignature() const 
        { 
            return m_pCommandSignature;
        }

    protected:
        std::unique_ptr<IndirectParameter[]> m_pIndirectParameters;
        ID3D12CommandSignature* m_pCommandSignature;

        BOOL m_bFinalized;
        UINT m_numParameters;
	};
}

namespace custom
{
    void CommandSignature::Finalize(const RootSignature* RootSignature /*= nullptr*/)
    {
        ASSERT(m_bFinalized);

        UINT ByteStride = 0;
        bool bRequiresRootSignature = false;

        for (UINT i = 0; i < m_numParameters; ++i)
        {
            switch (m_pIndirectParameters[i].GetDesc().Type)
            {
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
            {
                ByteStride += sizeof(D3D12_DRAW_ARGUMENTS);
                break;
            }
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
            {
                ByteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
                break;
            }
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
            {
                ByteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
                break;
            }
            case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
            {
                ByteStride += m_pIndirectParameters[i].GetDesc().Constant.Num32BitValuesToSet * 4;
                bRequiresRootSignature = true;
                break;
            }
            case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
            {
                ByteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
                break;
            }
            case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
            {
                ByteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
                break;
            }
            case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
                ByteStride += 8;
                bRequiresRootSignature = true;
                break;
            }
        }

        D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc;
        CommandSignatureDesc.ByteStride = ByteStride;
        CommandSignatureDesc.NumArgumentDescs = m_numParameters;
        CommandSignatureDesc.pArgumentDescs = (const D3D12_INDIRECT_ARGUMENT_DESC*)m_pIndirectParameters.get();
        CommandSignatureDesc.NodeMask = 1;

        Microsoft::WRL::ComPtr<ID3DBlob> pOutBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;

        ID3D12RootSignature* pRootSig = RootSignature ? RootSignature->GetSignature() : nullptr;

        if (bRequiresRootSignature)
        {
            ASSERT(pRootSig != nullptr);
        }
        else
        {
            pRootSig = nullptr;
        }

        ASSERT_HR
        (
            device::g_pDevice->CreateCommandSignature(
                &CommandSignatureDesc, pRootSig,
                IID_PPV_ARGS(&m_pCommandSignature))
        );

        SET_NAME(m_pCommandSignature);

        m_bFinalized = TRUE;
    }
}