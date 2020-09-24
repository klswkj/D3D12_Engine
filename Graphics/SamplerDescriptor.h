#pragma once
#include "stdafx.h"
#include "Color.h"


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

		void SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE AddressMode)
		{
			AddressU = AddressMode;
			AddressV = AddressMode;
			AddressW = AddressMode;
		}

		void SetBorderColor(custom::Color Border)
		{
			BorderColor[0] = Border.R();
			BorderColor[1] = Border.G();
			BorderColor[2] = Border.B();
			BorderColor[3] = Border.A();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE RequestHandle();

	private:
		size_t makeHash();
		void cleanUp();

	private:
		static std::unordered_map<size_t, D3D12_CPU_DESCRIPTOR_HANDLE> sm_samplerDescriptorHashMap;
		BOOL m_finalized{ false };
		uint64_t m_refCount= 0;

		struct SamplerDescriptorHash
		{
			SamplerDescriptorHash() = default;
			SamplerDescriptorHash(D3D12_SAMPLER_DESC& desc)
			{
				Filter = desc.Filter;
				TEXTURE_ADDRESS_MODE_U = desc.AddressU;
				TEXTURE_ADDRESS_MODE_V = desc.AddressV;
				TEXTURE_ADDRESS_MODE_W = desc.AddressW;
				MipLODBias = *reinterpret_cast<uint64_t*>(&desc.MipLODBias) >> 19ull;
				MaxAnisotropy = desc.MaxAnisotropy;
				COMPARISON_FUNC = desc.ComparisonFunc;
				BorderColorR = *reinterpret_cast<uint64_t*>(&desc.BorderColor[0]) >> 19ull;
				BorderColorB = *reinterpret_cast<uint64_t*>(&desc.BorderColor[1]) >> 19ull;
				BorderColorG = *reinterpret_cast<uint64_t*>(&desc.BorderColor[2]) >> 19ull;
				BorderColorA = *reinterpret_cast<uint64_t*>(&desc.BorderColor[3]) >> 19ull;
				MinLOD = *reinterpret_cast<uint64_t*>(&desc.MinLOD) >> 19ull;
				MaxLOD = *reinterpret_cast<uint64_t*>(&desc.MaxLOD);
			}

			SamplerDescriptorHash(custom::SamplerDescriptor& desc)
			{
				Filter = desc.Filter;
				TEXTURE_ADDRESS_MODE_U = desc.AddressU;
				TEXTURE_ADDRESS_MODE_V = desc.AddressV;
				TEXTURE_ADDRESS_MODE_W = desc.AddressW;
				MipLODBias = *reinterpret_cast<uint64_t*>(&desc.MipLODBias) >> 19ull;
				MaxAnisotropy = desc.MaxAnisotropy;
				COMPARISON_FUNC = desc.ComparisonFunc;
				BorderColorR = *reinterpret_cast<uint64_t*>(&desc.BorderColor[0]) >> 19ull;
				BorderColorB = *reinterpret_cast<uint64_t*>(&desc.BorderColor[1]) >> 19ull;
				BorderColorG = *reinterpret_cast<uint64_t*>(&desc.BorderColor[2]) >> 19ull;
				BorderColorA = *reinterpret_cast<uint64_t*>(&desc.BorderColor[3]) >> 19ull;
				MinLOD = *reinterpret_cast<uint64_t*>(&desc.MinLOD) >> 19ull;
				MaxLOD = *reinterpret_cast<uint64_t*>(&desc.MaxLOD);
			}

			union
			{
				struct
				{
					uint64_t Filter : 9;  // 9 -> 9         (9)
					uint64_t TEXTURE_ADDRESS_MODE_U : 3;  // 3 -> 3         (12)
					uint64_t TEXTURE_ADDRESS_MODE_V : 3;  // 3 -> 3         (15)
					uint64_t _Pad1 : 1;  //                (16)
					uint64_t TEXTURE_ADDRESS_MODE_W : 3;  // 3 -> 3         (19)
					uint64_t MipLODBias : 13; // 32(FLOAT) -> 8 (32)
					uint64_t MaxAnisotropy : 8;  // 32(UINT) -> 8  (40)
					uint64_t COMPARISON_FUNC : 4;  // 4 -> 4         (44)
					uint64_t BorderColorR : 13; // 32 -> 8        (57)
					uint64_t BorderColorB : 13; // 32 -> 8        (70)
					uint64_t BorderColorG : 13; // 32 -> 8        (83)
					uint64_t BorderColorA : 13; // 32 -> 8        (96)
					uint64_t MinLOD : 13; // 32 -> 8        (109)
					uint64_t MaxLOD : 13; // 32 -> 8        (122)
					uint64_t _Pad6 : 6;  //                (128)
				};
				struct
				{
					uint64_t Hash0;
					uint64_t Hash1;
				};
			};
		};
	};
}