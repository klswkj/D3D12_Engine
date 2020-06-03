
// DescriptorHeapAllocator
// ¡è 2. Check      ¦¢       
// Sampler Manager ¦¢  <¦¤
//   3. Allocation ¦¢   ¦¢ 1. Allocation Request
//                 ¡é   ¦¢
// Sampler Descriptor ¦¡¦¥
/////////////////////////////////////////////////////////
// DescriptorHeapAllocator m;
//
// SamplerDescriptor a;
// SamplerManager b(DeviceResourceManager.device(), m);
// 
// D3D12_CPU_HEAP_HANDLE temp = b.requestHandle(a);

#pragma once
#include "stdafx.h"

namespace custom
{
	class SamplerDescriptor : public D3D12_SAMPLER_DESC
	{
	public:
		SamplerDescriptor()
		{
			Filter = D3D12_FILTER_ANISOTROPIC;
			AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			MipLODBias = 0.0f;
			MaxAnisotropy = 16;
			ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			BorderColor[0] = 1.0f;
			BorderColor[1] = 1.0f;
			BorderColor[2] = 1.0f;
			BorderColor[3] = 1.0f;
			MinLOD = 0.0f;
			MaxLOD = D3D12_FLOAT32_MAX;
		}

		void Init(ID3D12Device* pDevice)
		{
			g_pDevice = pDevice;
		}

		void SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE AddressMode)
		{
			AddressU = AddressMode;
			AddressV = AddressMode;
			AddressW = AddressMode;
		}

		void SetBorderColor(Color Border)
		{
			BorderColor[0] = Border.R();
			BorderColor[1] = Border.G();
			BorderColor[2] = Border.B();
			BorderColor[3] = Border.A();
		}
	private:
		CONTAINED ID3D12Device* g_pDevice;

		BOOL m_finalized{ false };
		uint64_t m_refCount{ 0 };
	};
}