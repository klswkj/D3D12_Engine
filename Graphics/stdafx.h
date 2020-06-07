#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

#include <windowsx.h>
#include <Windows.h>
#include <wincodec.h>
#include <wrl.h>
#include <cstdint>
#include <cstdarg>
#include <ppltasks.h>
#include <exception>
#include <assert.h>
#include <sal.h>

#include <vector>
#include <string>
#include <queue>
#include <unordered_map>

#include <algorithm>
#include <utility>
#include <functional>
#include <memory>
#include <iterator>

#include <thread>
#include <mutex>

#include <dwrite_2.h>
#include <d2d1_3.h>

#include <dxgi1_4.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>
#include <d3d11on12.h>
#include "DDS.h"
#include "DDSTextureLoader.h"

#if defined(_DEBUG) || !defined(RELEASE)
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <pix3.h>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib, "D3DCompiler.lib")

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "resource.h"

#include "Hash.h"
// #include "WinDefine.h"
#include "TypeDefine.h"
#include "CustomAssert.h"

#define FILEACCESS
#define CONTAINED

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

template<typename T>
inline void SafeRelease(T*& rpInterface)
{
	T* pInterface = rpInterface;

	if (pInterface)
	{
		pInterface->Release();
		rpInterface = nullptr;
	}
}

template<typename T>
inline void SafeDelete(T*& rpObject)
{
	T* pObject = rpObject;

	if (pObject)
	{
		delete pObject;
		rpObject = nullptr;
	}
}


// Insecure Function.
// 
void SIMDMemCopy(void* __restrict _Dest, const void* __restrict _Source, size_t NumQuadwords)
{
    ASSERT(HashInternal::IsAligned(_Dest, 16));
    ASSERT(HashInternal::IsAligned(_Source, 16));

    __m128i* __restrict Dest = (__m128i * __restrict)_Dest;
    const __m128i* __restrict Source = (const __m128i * __restrict)_Source;

    // Discover how many quadwords precede a cache line boundary.  
    // Copy them separately.
    size_t InitialQuadwordCount = (4 - ((size_t)Source >> 4) & 3) & 3;
    if (NumQuadwords < InitialQuadwordCount)
    {
        InitialQuadwordCount = NumQuadwords;
    }

    if (0 < InitialQuadwordCount)
    {
        _mm_stream_si128(Dest + (InitialQuadwordCount - 1), _mm_load_si128(Source + (InitialQuadwordCount - 1)));
        // Fall through
    }

    if (NumQuadwords == InitialQuadwordCount)
    {
        return;
    }

    Dest += InitialQuadwordCount;
    Source += InitialQuadwordCount;
    NumQuadwords -= InitialQuadwordCount;

    size_t CacheLines = NumQuadwords >> 2;

    if (CacheLines)
    {
        _mm_prefetch((char*)(Source + (CacheLines - 1) * 9), _MM_HINT_NTA);    // Fall through

        // Do four quadwords per loop to minimize stalls.
        for (size_t i = CacheLines; 0 < i; --i)
        {
            // If this is a large copy, start prefetching future cache lines.  This also prefetches the
            // trailing quadwords that are not part of a whole cache line.
            if (10 <= i)
            {
                _mm_prefetch((char*)(Source + 40), _MM_HINT_NTA);
            }
            _mm_stream_si128(Dest + 0, _mm_load_si128(Source + 0));
            _mm_stream_si128(Dest + 1, _mm_load_si128(Source + 1));
            _mm_stream_si128(Dest + 2, _mm_load_si128(Source + 2));
            _mm_stream_si128(Dest + 3, _mm_load_si128(Source + 3));

            Dest += 4;
            Source += 4;
        }
    }

    // Copy the remaining quadwords
    if (NumQuadwords & 3)
    {
        _mm_stream_si128(Dest + (NumQuadwords & 3) - 1, _mm_load_si128(Source + (NumQuadwords & 3) - 1));
        // Fall through
    }

    _mm_sfence();
}

void SIMDMemFill(void* __restrict _Dest, __m128 FillVector, size_t NumQuadwords)
{
    ASSERT(HashInternal::IsAligned(_Dest, 16));
    // Keyword 'Register' is no longer a supported storage class
    /*register */const __m128i Source = _mm_castps_si128(FillVector);
    __m128i* __restrict Dest = (__m128i * __restrict)_Dest;

    if (((size_t)Dest >> 4) & 3)
    {
        _mm_stream_si128(Dest++, Source); 
        --NumQuadwords;
        // Fall through
    }

    size_t WholeCacheLines = NumQuadwords >> 2;

    // Do four quadwords per loop to minimize stalls.
    while (WholeCacheLines--)
    {
        _mm_stream_si128(Dest++, Source);
        _mm_stream_si128(Dest++, Source);
        _mm_stream_si128(Dest++, Source);
        _mm_stream_si128(Dest++, Source);
    }

    // Copy the remaining quadwords
    if (((size_t)Dest >> 4) & 3)
    {
        _mm_stream_si128(Dest++, Source);
        --NumQuadwords;
        // Fall through
    }

    _mm_sfence();
}


/*
#include "ReadyMadePSO.h"


class MyKeyboard;
class MyMouse;

class DescriptorHeapAllocator;

#include "MyKeyboard.h"
#include "MyMouse.h"

#include "RootParameter.h"
#include "RootSignature.h"
#include "Color.h"

#include "SamplerDescriptor.h"
#include "DescriptorHeapManager.h"
#include "SamplerManager.h"

#include "GPUResource.h"
#include "PixelBuffer.h"
#include "UAVBuffer.h"

#include "LinearAllocationPage.h"
#include "LinearAllocator.h"

#include "CommandContext.h"
#include "DDSTextureLoader.cpp"
#include "Texture.h"
#include "TextureManager.h"

#include "CommandAllocatorPool.h"
#include "CommandQueue.h"
#include "CommandQueueManager.h"

#include "DescriptorHandle.h"
#include "DescriptorHeap.h"
#include "SVDescriptor.h"
#include "GraphicsDeviceManager.h"

#include "CommandContextManager.h"

#include "Graphics.h"
#include "Window.h"

struct pair_hash
{
	template <class T1, class T2>
	std::size_t operator() (const std::pair<T1, T2>& pair) const
	{
		return (std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second)) << (std::hash<T2>()((pair.second) | 0xff));
	}
};

struct GraphicsPSOHash
{
	GraphicsPSOHash() = default;
	GraphicsPSOHash(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		pRootSignature = reinterpret_cast<uint64_t>(desc.pRootSignature);
		VS = desc.VS.BytecodeLength;
		PS = desc.PS.BytecodeLength;
		DS = desc.DS.BytecodeLength;
		HS = desc.HS.BytecodeLength;

		BlendState = (desc.BlendState.RenderTarget[0].BlendEnable << 17ull)   |
			         (desc.BlendState.RenderTarget[0].LogicOpEnable << 16ull) |
			         (desc.BlendState.RenderTarget[0].BlendOp << 13ull)       |
			         (desc.BlendState.RenderTarget[0].BlendOpAlpha << 10ull)  |
			         (desc.BlendState.RenderTarget[0].SrcBlend << 5ull)       |
			         (desc.BlendState.RenderTarget[0].DestBlend);

		DepthStencilState = (desc.DepthStencilState.DepthEnable << 4ull)    |
						    (desc.DepthStencilState.DepthWriteMask << 3ull) |
						    (desc.DepthStencilState.DepthFunc - 1ull);

		RasterizerState = (desc.RasterizerState.FillMode) << 30ull |
						  (desc.RasterizerState.CullMode) << 28ull |
						  (desc.RasterizerState.MultisampleEnable) << 27ull |
						  (desc.RasterizerState.FrontCounterClockwise) << 26ull |
						  (*reinterpret_cast<uint64_t*>(&desc.RasterizerState.SlopeScaledDepthBias) >> 19ull) << 13ull |
						  (*reinterpret_cast<uint64_t*>(&desc.RasterizerState.DepthBias) >> 19ull);

        InputLayout     = *reinterpret_cast<uint64_t*>(&desc.InputLayout.pInputElementDescs);
		RTVFormat       = *desc.RTVFormats;
		DSVFormat       = desc.DSVFormat;
		PrimitiveTopologyType = desc.PrimitiveTopologyType;
	}
	union
	{
		// http://timjones.io/blog/archive/2015/09/02/parsing-direct3d-shader-bytecode
		struct
		{
			uint64_t pRootSignature : 8;           // 32 or 64 -> 8    (8)
			uint64_t VS : 8;                       // 4k -> 8          (16)
			uint64_t PS : 8;                       // 4k -> 8          (24)
			uint64_t DS : 8;                       // 4k -> 8          (32)
			uint64_t HS : 8;                       // 4k -> 8          (40)
			uint64_t BlendState            : 18;   // 2 + 44 -> 8     (58)
			uint64_t DepthStencilState     : 5;    // DepthEnable(1), DepthWriteMask(1), DepthFunc(3) -> 5 (63)
			uint64_t RasterizerState       : 31;   // Fillmode(1), CullMode(2), MultiSamplerEnable(1) 
												   // FrontCounterClockwise(1), sloperScaledDepthBias(13), DepthBias(13) -> 31 (94)
			uint64_t InputLayout           : 8;    // 32 or 64 -> 9    (102)
			uint64_t RTVFormat             : 8;    // 9 -> 8           (110)
			uint64_t DSVFormat             : 8;    // 9 -> 8           (118)
			uint64_t PrimitiveTopologyType : 3;    // 3 -> 3           (121)
			uint64_t _Pad7 : 7;
		};
		struct
		{
			uint64_t Hash0;
			uint64_t Hash1;
		};
	};
};

struct RootSignatureHash
{
	// RootSignature는 D3D12 Device 내부적으로 똑같은게 있으면, RefCount 올려주고 같은 걸 넘겨주므로 패스
};

*/