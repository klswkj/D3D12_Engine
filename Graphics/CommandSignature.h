#pragma once
#include "stdafx.h"
#include "Device.h"
#include "IndirectParameter.h"

namespace custom
{
    class RootSignature;

	class CommandSignature
	{
    public:
        CommandSignature(UINT NumParams = 0) 
            : 
            m_bFinalized(false), 
            m_numParameters(NumParams)
        {
            Reset(NumParams);
        }

        void Destroy()
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