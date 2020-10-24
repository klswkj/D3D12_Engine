#pragma once

namespace custom
{
	class RootParameter
	{
		friend class RootSignature;
	public:
		RootParameter()
		{
			m_D3D12RootParameter.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
			m_D3D12RootParameter.DescriptorTable.pDescriptorRanges = nullptr;
		}

		~RootParameter()
		{
			Clear();
		}

		void Clear()
		{

			if (
				m_D3D12RootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && 
				m_D3D12RootParameter.DescriptorTable.pDescriptorRanges != nullptr
				)
			{
				delete[] m_D3D12RootParameter.DescriptorTable.pDescriptorRanges;
				m_D3D12RootParameter.DescriptorTable.pDescriptorRanges = nullptr;
			}
			
			m_D3D12RootParameter.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
		}

		void InitAsConstants(uint32_t Register, uint32_t NumDwords, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_D3D12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			m_D3D12RootParameter.ShaderVisibility = Visibility;
			m_D3D12RootParameter.Constants.Num32BitValues = NumDwords;
			m_D3D12RootParameter.Constants.ShaderRegister = Register;
			m_D3D12RootParameter.Constants.RegisterSpace = 0;
		}

		void InitAsConstantBuffer(uint32_t Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_D3D12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			m_D3D12RootParameter.ShaderVisibility = Visibility;
			m_D3D12RootParameter.Descriptor.ShaderRegister = Register;
			m_D3D12RootParameter.Descriptor.RegisterSpace = 0;
		}

		void InitAsBufferSRV(uint32_t Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_D3D12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			m_D3D12RootParameter.ShaderVisibility = Visibility;
			m_D3D12RootParameter.Descriptor.ShaderRegister = Register;
			m_D3D12RootParameter.Descriptor.RegisterSpace = 0;
		}

		void InitAsBufferUAV(uint32_t Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_D3D12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			m_D3D12RootParameter.ShaderVisibility = Visibility;
			m_D3D12RootParameter.Descriptor.ShaderRegister = Register;
			m_D3D12RootParameter.Descriptor.RegisterSpace = 0;
		}

		void InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32_t Register, uint32_t Count, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsDescriptorTable(1, Visibility);
			SetTableRange(0, Type, Register, Count);
		}

		void InitAsDescriptorTable(uint32_t RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_D3D12RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			m_D3D12RootParameter.ShaderVisibility = Visibility;
			m_D3D12RootParameter.DescriptorTable.NumDescriptorRanges = RangeCount;
			m_D3D12RootParameter.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
		}

		void SetTableRange(uint32_t RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32_t Register, uint32_t Count, uint32_t Space = 0)
		{
			D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(m_D3D12RootParameter.DescriptorTable.pDescriptorRanges + RangeIndex);
			range->RangeType = Type;
			range->NumDescriptors = Count;
			range->BaseShaderRegister = Register;
			range->RegisterSpace = Space;
			range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		}

		const D3D12_ROOT_PARAMETER& operator() () const 
		{ 
			return m_D3D12RootParameter;
		}
	protected:
		D3D12_ROOT_PARAMETER m_D3D12RootParameter;
	};
}

/*
#pragma once

namespace custom
{
	class RootParameter
	{
		friend class RootSignature;
	public:
		RootParameter()
		{
			m_RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
		}

		~RootParameter()
		{
			Clear();
		}

		void Clear()
		{
			// 여기서 해야될듯.
			// 레퍼런스형으로? 레퍼런스 카운트해서 delete하는걸로?

			if (m_RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				delete[] m_RootParam.DescriptorTable.pDescriptorRanges;
			}

			m_RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
		}

		void InitAsConstants(uint32_t Register, uint32_t NumDwords, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			m_RootParam.ShaderVisibility = Visibility;
			m_RootParam.Constants.Num32BitValues = NumDwords;
			m_RootParam.Constants.ShaderRegister = Register;
			m_RootParam.Constants.RegisterSpace = 0;
		}

		void InitAsConstantBuffer(uint32_t Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			m_RootParam.ShaderVisibility = Visibility;
			m_RootParam.Descriptor.ShaderRegister = Register;
			m_RootParam.Descriptor.RegisterSpace = 0;
		}

		void InitAsBufferSRV(uint32_t Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			m_RootParam.ShaderVisibility = Visibility;
			m_RootParam.Descriptor.ShaderRegister = Register;
			m_RootParam.Descriptor.RegisterSpace = 0;
		}

		void InitAsBufferUAV(uint32_t Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			m_RootParam.ShaderVisibility = Visibility;
			m_RootParam.Descriptor.ShaderRegister = Register;
			m_RootParam.Descriptor.RegisterSpace = 0;
		}

		void InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32_t Register, uint32_t Count, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsDescriptorTable(1, Visibility);
			SetTableRange(0, Type, Register, Count);
		}

		void InitAsDescriptorTable(uint32_t RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			m_RootParam.ShaderVisibility = Visibility;
			m_RootParam.DescriptorTable.NumDescriptorRanges = RangeCount;
			m_RootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
		}

		void SetTableRange(uint32_t RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32_t Register, uint32_t Count, uint32_t Space = 0)
		{
			D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(m_RootParam.DescriptorTable.pDescriptorRanges + RangeIndex);
			range->RangeType = Type;
			range->NumDescriptors = Count;
			range->BaseShaderRegister = Register;
			range->RegisterSpace = Space;
			range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		}

		const D3D12_ROOT_PARAMETER& operator() () const
		{
			return m_RootParam;
		}
	protected:
		D3D12_ROOT_PARAMETER m_RootParam;
	};
}
*/