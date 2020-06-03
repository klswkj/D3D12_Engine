#include "stdafx.h"
#include "GraphicsDeviceManager.h"

inline LPCSTR GetFeatureLevelName(D3D_FEATURE_LEVEL FeatureLevel)
{
    switch (FeatureLevel)
    {
    case D3D_FEATURE_LEVEL_9_1: return "9.1";
    case D3D_FEATURE_LEVEL_9_2: return "9.2";
    case D3D_FEATURE_LEVEL_9_3: return "9.3";
    case D3D_FEATURE_LEVEL_10_0: return "10.0";
    case D3D_FEATURE_LEVEL_10_1: return "10.1";
    case D3D_FEATURE_LEVEL_11_0: return "11.0";
    case D3D_FEATURE_LEVEL_11_1: return "11.1";
    case D3D_FEATURE_LEVEL_12_0: return "12.0";
    case D3D_FEATURE_LEVEL_12_1: return "12.1";

    default: return "12.1+";
    }
}

HRESULT GraphicsDeviceManager::createDeviceIndependentStateInternal()
{
    HRESULT hardwareResult;
    uint32_t dxgiFactoryFlags = 0;

#if(_DEBUG)
    {
        bool bDebugLayerEnabled = false;

        ID3D12Debug* pDebugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
        {
            pDebugController->EnableDebugLayer();
            pDebugController->Release();

            bDebugLayerEnabled = true;
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }

        wprintf(L"\nDebug layer is %senabled\n\n", bDebugLayerEnabled ? L"" : L"NOT ");
    }
#endif

    hardwareResult = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&g_pDXGIFactory));
    
    ASSERT_HR(hardwareResult);

    // QPC information for framerate tracking.
    QueryPerformanceFrequency(&m_PerformanceFrequency);
    QueryPerformanceCounter(&m_LastFrameCounter);

    // Initialize WIC.
    // 
    ASSERT_HR(CoInitialize(nullptr));

    ASSERT_HR(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_pWICFactory)));

    // Initialize D2D and DWrite device independent interfaces.
    //
    ASSERT_HR(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), (IUnknown**)&m_pDWriteFactory));

    ASSERT_HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&m_pD2DFactory)));

    ASSERT_HR(createDeviceIndependentState());

    return S_OK;
}

HRESULT GraphicsDeviceManager::createDeviceIndependentState()
{
    HRESULT hardwareResult;

   ASSERT_HR
   (
       m_pDWriteFactory->CreateTextFormat(
        L"Verdana",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        11.0f,
        L"en-us",
        &m_pTextFormat)
   );

    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return S_OK;
}

HRESULT GraphicsDeviceManager::createDeviceDependentStateInternal()
{
    HRESULT hardwareResult;
    {
        // Have the user select the desired graphics adapter.
        // 
        IDXGIAdapter* pTempAdapter;
        uint32_t AdapterOrdinal = 0;

        printf("Available Adapters:\n");
        while (g_pDXGIFactory->EnumAdapters(AdapterOrdinal, &pTempAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC Desc;
            pTempAdapter->GetDesc(&Desc);

            printf("  [%d] %ls\n", AdapterOrdinal, Desc.Description);

            pTempAdapter->Release();
            ++AdapterOrdinal;
        }

        while(1)
        {
            printf("\nSelect Adapter: ");

            AdapterOrdinal = m_NewAdapterIndex;
            if (AdapterOrdinal == 0xFFFFFFFF)
            {
                if (scanf_s("%d", &AdapterOrdinal) == 0)
                {
                    // If the user did not specify valid input, just use adapter 0.
                    //
                    AdapterOrdinal = 0;
                }
            }

            ASSERT_HR(g_pDXGIFactory->EnumAdapters(AdapterOrdinal, &pTempAdapter));

            ASSERT_HR(pTempAdapter->QueryInterface(&g_pDXGIAdapter));
            pTempAdapter->Release();

            break;
        }
    }

    // Obtain the default video memory information for the local and non-local segment groups.
    //
    ASSERT_HR(g_pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_LocalVideoMemoryInfo));

    ASSERT_HR(g_pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &m_NonLocalVideoMemoryInfo));

    ASSERT_HR(D3D12CreateDevice(g_pDXGIAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_pDevice)));

#if defined(_DEBUG)
    ID3D12InfoQueue* pInfoQueue = nullptr;

    ASSERT_HR(g_pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue)));

    {
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID.
        D3D12_MESSAGE_ID DenyIds[] =
        {
            // The 11On12 implementation does not use optimized clearing yet.
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        pInfoQueue->PushStorageFilter(&NewFilter);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        pInfoQueue->Release();
    }
#endif

    // Cache the descriptor sizes.
    //
    m_descriptorInfo.RtvDescriptorSize = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_descriptorInfo.SamplerDescriptorSize = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_descriptorInfo.SrvUavCbvDescriptorSize = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ASSERT_HR(queryCaps());

    // Create a basic Flip-Discard swapchain.
    //
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.BufferCount = SWAPCHAIN_BUFFER_COUNT;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.Scaling = DXGI_SCALING_NONE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.Height = m_WindowHeight;
    SwapChainDesc.Width = m_WindowWidth;
    SwapChainDesc.Flags = 0;

    //
    // The swap chain is created as an IDXGISwapChain1 interface, but we then query
    // the IDXGISwapchain3 interface from it.
    //
    IDXGISwapChain1* pSwapChain;
    hardwareResult = g_pDXGIFactory->CreateSwapChainForHwnd(m_RenderContext.GetCommandQueue(), m_Hwnd, &SwapChainDesc, nullptr, nullptr, &pSwapChain);
    if (FAILED(hardwareResult))
    {
        LOG_ERROR("Failed to create swap chain for hwnd, hr=0x%.8x", hardwareResult);
        return hardwareResult;
    }

    hardwareResult = pSwapChain->QueryInterface(&g_pDXGISwapChain);
    pSwapChain->Release();
    if (FAILED(hardwareResult))
    {
        LOG_ERROR("Failed to query IDXGISwapChain3 interface, hr=0x%.8x", hardwareResult);
        return hardwareResult;
    }

   
    // Create 11On12 state to enable D2D rendering on D3D12.
    //
    IUnknown* pRenderCommandQueue = m_RenderContext.GetCommandQueue();
    ASSERT_HR
    (
        D3D11On12CreateDevice
        (
            g_pDevice, D3D11_CREATE_DEVICE_BGRA_SUPPORT, 
            nullptr, 0, &pRenderCommandQueue, 1, 0, &m_p11Device, &m_p11Context, nullptr
        )
    );

    ASSERT_HR(m_p11Device->QueryInterface(&m_p11On12Device));

    // Create D2D/DWrite components.
    //
    {
        D2D1_DEVICE_CONTEXT_OPTIONS DeviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
        IDXGIDevice* pDxgiDevice;
        ASSERT_HR(m_p11On12Device->QueryInterface(&pDxgiDevice));

        ASSERT_HR(m_pD2DFactory->CreateDevice(pDxgiDevice, &m_pD2DDevice));
        pDxgiDevice->Release();

        ASSERT_HR(m_pD2DDevice->CreateDeviceContext(DeviceOptions, &m_pD2DContext));
    }

    // Create the resources to enable D2D to access the swap chain.
    //
    ASSERT_HR(createSwapChainResources());

    ASSERT_HR(m_pD2DContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pTextBrush));

    return S_OK;
}

HRESULT GraphicsDeviceManager::createSwapChainResources()
{
    float dpiX;
    float dpiY;
    //m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1
    (
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        800, 600
    );

    // For each buffer in the swapchain, we need to create a wrapped resource, and create
    // a D2D render target object to enable D2D rendering for the UI.
    //
    for (size_t n = 0; n < SWAPCHAIN_BUFFER_COUNT; n++)
    {
        ASSERT_HR(g_pDXGISwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n])));

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(), n, m_DescriptorInfo.RtvDescriptorSize);
        g_pDevice->CreateRenderTargetView(m_pRenderTargets[n], nullptr, handle);

        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
        hr = m_p11On12Device->CreateWrappedResource(
            m_pRenderTargets[n],
            &d3d11Flags,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT,
            IID_PPV_ARGS(&m_pWrappedBackBuffers[n]));
        if (FAILED(hr))
        {
            LOG_ERROR("Failed create 11On12 wrapped resource for render target %d, hr=0x%.8x", n, hr);
            return hr;
        }

        IDXGISurface* pSurface;
        hr = m_pWrappedBackBuffers[n]->QueryInterface(&pSurface);
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to query IDXGISurface from wrapped back buffer, hr=0x%.8x", hr);
            return hr;
        }

        hr = m_pD2DContext->CreateBitmapFromDxgiSurface(pSurface, &bitmapProperties, &m_pD2DRenderTargets[n]);
        pSurface->Release();
        if (FAILED(hr))
        {
            LOG_ERROR("Failed to create D2D bitmap from back buffer surface, hr=0x%.8x", hr);
            return hr;
        }
    }

    return S_OK;
}

HRESULT GraphicsDeviceManager::queryCaps()
{
    ASSERT_HR(g_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &m_Options, sizeof(m_Options)));

    ASSERT_HR(g_pDevice->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &m_GpuVaSupport, sizeof(m_GpuVaSupport)));

    D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevels = {};
    D3D_FEATURE_LEVEL FeatureLevelsList[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1,
    };
    FeatureLevels.NumFeatureLevels = ARRAYSIZE(FeatureLevelsList);
    FeatureLevels.pFeatureLevelsRequested = FeatureLevelsList;

    ASSERT_HR(g_pDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevels, sizeof(FeatureLevels)));

    printf("\n");
    printf("--- Device details --------------------\n");
    printf("  GPU Virtual Address Info:\n");
    printf("    Max Process Size:      %I64d GB\n", (1ULL << m_GpuVaSupport.MaxGPUVirtualAddressBitsPerProcess) / 1024 / 1024 / 1024);
    printf("    Max Resource Size:     %I64d GB\n", (1ULL << m_GpuVaSupport.MaxGPUVirtualAddressBitsPerResource) / 1024 / 1024 / 1024);
    printf("  Feature Level Info:\n");
    printf("    Max Feature Level:     %s\n", GetFeatureLevelName(FeatureLevels.MaxSupportedFeatureLevel));
    printf("  Additional Info:\n");
    printf("    Resource Binding Tier: Tier %d\n", m_Options.ResourceBindingTier);
    printf("    Resource Heap Tier:    Tier %d\n", m_Options.ResourceHeapTier);
    printf("    Tiled Resource Tier:   Tier %d\n", m_Options.TiledResourcesTier);
    printf("\n");

    return S_OK;
}

HRESULT GraphicsDeviceManager::recreateDeviceDependentState()
{
    // 다 파괴하고 다시
    // Init()부르기
}
