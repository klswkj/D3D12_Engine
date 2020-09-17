#pragma once

namespace custom
{
	class IndirectParameter : public D3D12_INDIRECT_ARGUMENT_DESC
	{
        friend class CommandSignature;

    public:
        IndirectParameter()
        {
            Type = (D3D12_INDIRECT_ARGUMENT_TYPE)0xFFFFFFFF;
        }

        void SetDraw()
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
        }

        void SetDrawIndexed()
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
        }

        void SetDispatch()
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
        }

        void SetVertexBufferView(UINT Slot)
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
            VertexBuffer.Slot = Slot;
        }

        void SetIndexBufferView()
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
        }

        void SetConstant(UINT RootParameterIndex, UINT DestOffsetIn32BitValues, UINT Num32BitValuesToSet)
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
            Constant.RootParameterIndex = RootParameterIndex;
            Constant.DestOffsetIn32BitValues = DestOffsetIn32BitValues;
            Constant.Num32BitValuesToSet = Num32BitValuesToSet;
        }

        void SetConstantBufferView(UINT RootParameterIndex)
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
            ConstantBufferView.RootParameterIndex = RootParameterIndex;
        }

        void SetShaderResourceView(UINT RootParameterIndex)
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
            ShaderResourceView.RootParameterIndex = RootParameterIndex;
        }

        void SetUnorderedAccessView(UINT RootParameterIndex)
        {
            Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
            UnorderedAccessView.RootParameterIndex = RootParameterIndex;
        }

        const IndirectParameter& GetDesc() const
        { 
            return *this; 
        }
	};
}

