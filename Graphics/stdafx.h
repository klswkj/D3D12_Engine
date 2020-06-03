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
#include "WinDefine.h"
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