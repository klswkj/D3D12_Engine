#pragma once
#include "Window.h" // for g_windowHeight, g_windowWidth.
#include "PSO.h"
#include "RootSignature.h"
#include "ColorBuffer.h"

class ColorBuffer; // for SwapChainResource
class DepthBuffer;
class GraphicsPSO;
class CommandContextManager;
class CommandQueueManager;
class CommandSignature;
class ContextManager;
class DescriptorHeapManager;
// GraphicsCore.h
#pragma once
#include "stdafx.h"

class ColorBuffer; // for SwapChainResource
class CommandContextManager;
class CommandQueueManager;
class CommandSignature;
class ContextManager;
class DescriptorHeapManager;

namespace device
{
    HRESULT Initialize();
    HRESULT Resize(uint32_t width, uint32_t height);
    void Terminate();
    void Shutdown();
    void Present();

    uint64_t GetFrameCount();
    float GetFrameTime();
    float GetFrameRate();

    extern ID3D12Device* g_pDevice;
    extern CommandQueueManager   g_commandQueueManager;
    extern CommandContextManager g_commandContextManager;
    extern DescriptorHeapManager g_descriptorHeapManager;

    extern ColorBuffer g_DisplayBuffer[];

    extern uint32_t g_DescriptorSize[];

    extern D3D_FEATURE_LEVEL g_D3DFeatureLevel;
    extern D3D12_FEATURE_DATA_D3D12_OPTIONS g_Options;
    extern D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT g_GpuVaSupport;
    extern DXGI_QUERY_VIDEO_MEMORY_INFO g_LocalVideoMemoryInfo;
    extern DXGI_QUERY_VIDEO_MEMORY_INFO g_NonLocalVideoMemoryInfo;

    extern bool g_bTypedUAVLoadSupport_R11G11B10_FLOAT;
    extern bool g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT;

    extern IDXGISwapChain3* g_pDXGISwapChain;

    extern UINT g_DisplayBufferCount;

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern Platform::Agile<Windows::UI::Core::CoreWindow>  g_window;
#endif
}

namespace GraphicsDeviceManager
{
    HRESULT queryCaps();
    HRESULT createDeviceDependentStateInternal();
}