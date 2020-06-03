#pragma once

static constexpr size_t SWAPCHAIN_BUFFER_COUNT = 3;
static constexpr size_t STATISTIC_COUNT = 60;

// Structure to save off descriptor sizes for various descriptor heap types.
// These values never change, and need only be queried by the D3D runtime once.
//
struct DescriptorInfo
{
    UINT32 RtvDescriptorSize;
    UINT32 SamplerDescriptorSize;
    UINT32 SrvUavCbvDescriptorSize;
};

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


    inline const DXGI_QUERY_VIDEO_MEMORY_INFO& GetLocalVideoMemoryInfo() const
    {
        return m_LocalVideoMemoryInfo;
    }

    inline const DXGI_QUERY_VIDEO_MEMORY_INFO& GetNonLocalVideoMemoryInfo() const
    {
        return m_NonLocalVideoMemoryInfo;
    }


    BOOL IsUnderBudget() const
    {
        return m_LocalVideoMemoryInfo.CurrentUsage <= m_LocalVideoMemoryInfo.Budget;
    }

    BOOL IsOverBudget() const
    {
        return m_LocalVideoMemoryInfo.CurrentUsage > m_LocalVideoMemoryInfo.Budget;
    }

    BOOL IsWithinBudgetThreshold(uint64_t Size) const
    {
        return m_LocalVideoMemoryInfo.CurrentUsage + Size <= m_LocalVideoMemoryInfo.Budget;
    }
    //
    // Statistics
    //
    inline uint32_t GetGlitchCount() const
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
    ID2D1DeviceContext2* m_pD2DContext = nullptr;
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