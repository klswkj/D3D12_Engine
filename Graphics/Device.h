#pragma once
#include "Window.h" // for g_windowHeight, g_windowWidth.
#include "PSO.h"
// #include "DescriptorHeapManager.h"
#include "RootSignature.h"
#include "ReadyMadePSO.h"

class ColorBuffer; // for SwapChainResource
class DepthBuffer;
class GraphicsPSO;
class CommandContextManager;
class CommandQueueManager;
class CommandSignature;
class ContextManager;
class DescriptorHeapManager;

namespace device
{
    HRESULT Initialize(void);
    HRESULT Resize(uint32_t width, uint32_t height);
    void Terminate(void);
    void Shutdown(void);
    void Present(void);

    extern ID3D12Device*       g_pDevice;
    // 일단은 static 따른 곳에서 쓰면 extern으로 바꾸고, cpp에서 직접적으로 선언할 것.
    static IDXGIFactory2*       s_pDXGIFactory  { nullptr };
    static IWICImagingFactory*  s_pWICFactory   { nullptr };
    static IDWriteFactory2*     s_pDWriteFactory{ nullptr };
    static ID2D1Factory3*       s_pD2DFactory   { nullptr };
    static IDXGIAdapter3*       s_pDXGIAdapter  { nullptr };
    static IDXGISwapChain3*     s_pDXGISwapChain{ nullptr };

    static ID3D11Device*        s_p11Device     { nullptr };
    static ID3D11DeviceContext* s_p11Context    { nullptr };
    static ID3D11On12Device*    s_p11On12Device { nullptr };
    static ID2D1Device2*        s_pD2DDevice    { nullptr };
    static ID2D1DeviceContext2* s_pD2DContext   { nullptr };

    extern CommandQueueManager   g_commandQueueManager;
    extern CommandContextManager g_commandContextManager;
    extern DescriptorHeapManager g_descriptorHeapManager;

    extern custom::RootSignature g_GenerateMipsRS;
    extern ComputePSO            g_GenerateMipsLinearPSO[4];
    extern ComputePSO            g_GenerateMipsGammaPSO[4];
    //
    // D2D UI rendering
    //
    static ID2D1SolidColorBrush* s_pTextBrush   { nullptr };
    static IDWriteTextFormat*    s_pTextFormat  { nullptr };

    static constexpr size_t SWAPCHAIN_BUFFER_COUNT = 3;
    static constexpr size_t STATISTIC_COUNT = 60;

    static bool g_bUseSharedStagingSurface = true;
    static bool g_bPresentOnVsync = true;

    static HRESULT g_SimulatedRenderResult = S_OK;
    static uint32_t g_NewAdapterIndex = 0xFFFFFFFF;

    extern uint32_t g_DescriptorSize[];

    extern DXGI_QUERY_VIDEO_MEMORY_INFO g_LocalVideoMemoryInfo;
    extern DXGI_QUERY_VIDEO_MEMORY_INFO g_NonLocalVideoMemoryInfo;
}

/*
namespace GraphicsDeviceManager
{
    HRESULT Init();

    //
    // Device state management
    //
    HRESULT createDeviceIndependentStateInternal();
    HRESULT createDeviceIndependentState();
    HRESULT createDeviceDependentStateInternal();
    HRESULT createSwapChainResources();
    HRESULT queryCaps();

    HRESULT recreateDeviceDependentState();

    //
    // Data Access
    //
    inline ID3D12Device* GetDevice()
    {
        return g_pDevice;
    }

    inline IDXGIAdapter3* GetAdapter()
    {
        return g_pDXGIAdapter;
    }


    inline const DXGI_QUERY_VIDEO_MEMORY_INFO& GetLocalVideoMemoryInfo()
    {
        return m_LocalVideoMemoryInfo;
    }

    inline const DXGI_QUERY_VIDEO_MEMORY_INFO& GetNonLocalVideoMemoryInfo()
    {
        return m_NonLocalVideoMemoryInfo;
    }


    BOOL IsUnderBudget()
    {
        return m_LocalVideoMemoryInfo.CurrentUsage <= m_LocalVideoMemoryInfo.Budget;
    }

    BOOL IsOverBudget()
    {
        return m_LocalVideoMemoryInfo.CurrentUsage > m_LocalVideoMemoryInfo.Budget;
    }

    BOOL IsWithinBudgetThreshold(uint64_t Size)
    {
        return m_LocalVideoMemoryInfo.CurrentUsage + Size <= m_LocalVideoMemoryInfo.Budget;
    }
    //
    // Statistics
    //
    inline uint32_t GetGlitchCount()
    {
        return m_GlitchCount;
    }

    IWICImagingFactory* g_pWICFactory = nullptr;
    IDXGIFactory2* g_pDXGIFactory = nullptr;
    IDXGIAdapter3* g_pDXGIAdapter = nullptr;
    ID3D12Device* g_pDevice = nullptr;
    IDXGISwapChain3* g_pDXGISwapChain = nullptr;
    ID3D12Resource* m_pRenderTargets[SWAPCHAIN_BUFFER_COUNT];
    ID3D12Resource* m_pStagingSurface = nullptr;
    void* m_pStagingSurfaceData = nullptr;

    //
    // 11On12 interface for UI
    //
    ID3D11Device* m_p11Device = nullptr;
    ID3D11DeviceContext* m_p11Context = nullptr;
    ID2D1DeviceContext2* m_pD2DContext = nullptr;
    ID3D11On12Device* m_p11On12Device = nullptr;
    ID2D1Factory3* m_pD2DFactory = nullptr;
    ID2D1Device2* m_pD2DDevice = nullptr;
    IDWriteFactory2* m_pDWriteFactory = nullptr;

    ID3D11Resource* m_pWrappedBackBuffers[SWAPCHAIN_BUFFER_COUNT];
    ID2D1Bitmap1* m_pD2DRenderTargets[SWAPCHAIN_BUFFER_COUNT];

    D3D12_FEATURE_DATA_D3D12_OPTIONS m_Options;
    D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT m_GpuVaSupport;

    DXGI_QUERY_VIDEO_MEMORY_INFO m_LocalVideoMemoryInfo;
    DXGI_QUERY_VIDEO_MEMORY_INFO m_NonLocalVideoMemoryInfo;

    //
    // D2D UI rendering
    //
    ID2D1SolidColorBrush* m_pTextBrush = nullptr;
    IDWriteTextFormat* m_pTextFormat = nullptr;

    //
    // Perf stats
    //
    uint32_t m_StatIndex = 0;
    LARGE_INTEGER m_PerformanceFrequency = {};
    LARGE_INTEGER m_LastFrameCounter = {};
    float m_StatTimeBetweenFrames[STATISTIC_COUNT];
    float m_StatRenderScene[STATISTIC_COUNT];
    float m_StatRenderUI[STATISTIC_COUNT];
    uint32_t m_PreviousPresentCount = 0;
    uint32_t m_PreviousRefreshCount = 0;
    uint32_t m_GlitchCount = 0;

    DescriptorInfo m_descriptorInfo;

    bool m_bUseSharedStagingSurface = true;
    bool m_bPresentOnVsync = true;

    HRESULT m_SimulatedRenderResult = S_OK;
    uint32_t m_NewAdapterIndex = 0xFFFFFFFF;
};
*/