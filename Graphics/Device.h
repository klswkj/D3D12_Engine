#pragma once
#include "Window.h"
// #include "ColorBuffer.h"

class ColorBuffer;
class DepthBuffer;
class CommandContextManager;
class CommandQueueManager;
class CommandSignature;
class ContextManager;
class DescriptorHeapManager;

namespace D3DX12Residency
{
    class ResidencyManager;
}

namespace device
{
    HRESULT Initialize();
    HRESULT Resize(uint32_t width, uint32_t height);
    void Destroy();

    HRESULT queryCaps();
    HRESULT CreateD3D12Devices();

    // extern Microsoft::WRL::ComPtr<IDXGIFactory2> g_pDXGIFactory;
    extern Microsoft::WRL::ComPtr<IDXGIAdapter3> g_pDXGIAdapter;
    // *Not* BitMask, only Index.
    extern UINT g_AdapterIndex; 

    extern ID3D12Device*         g_pDevice;
    extern CommandQueueManager   g_commandQueueManager;
    extern CommandContextManager g_commandContextManager;
    extern DescriptorHeapManager g_descriptorHeapManager;
    extern D3DX12Residency::ResidencyManager g_D3D12ResidencyManager;

    extern ColorBuffer g_DisplayColorBuffer[];

    extern uint32_t                                       g_DescriptorSize[];
    extern DXGI_FORMAT                                    g_SwapChainFormat;
    extern DXGI_QUERY_VIDEO_MEMORY_INFO                   g_LocalVideoMemoryInfo;
    extern DXGI_QUERY_VIDEO_MEMORY_INFO                   g_NonLocalVideoMemoryInfo;
    extern D3D_FEATURE_LEVEL                              g_D3DFeatureLevel;
    extern D3D12_FEATURE_DATA_D3D12_OPTIONS               g_Options;
    extern D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT g_GpuVaSupport;

    extern bool g_bTypedUAVLoadSupport_R11G11B10_FLOAT;
    extern bool g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT;

    extern IDXGISwapChain3*      g_pDXGISwapChain;
    extern HANDLE                g_hSwapChainWaitableObject;
    extern ID3D12DescriptorHeap* g_ImguiFontHeap;

    extern const UINT     g_DisplayBufferCount;
    extern       uint32_t g_DisplayWidth;
    extern       uint32_t g_DisplayHeight;


#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern Platform::Agile<Windows::UI::Core::CoreWindow>  g_window;
#endif
}