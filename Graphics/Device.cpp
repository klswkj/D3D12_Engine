#include "stdafx.h"
#include "Device.h"
#include "Window.h"
#include "PreMadePSO.h"
#include "BufferManager.h"
#include "CommandQueueManager.h"
#include "CommandQueue.h"
#include "CommandContextManager.h"
#include "CommandContext.h"
#include "Rootsignature.h"
#include "DescriptorHeapManager.h"
#include "DynamicDescriptorHeap.h"
#include "SystemTime.h"
#include "GpuTime.h"

// HRESULT Initialize()
// HRESULT Resize(uint32_t width, uint32_t height)
// void Terminate()
// void Destory()

// uint64_t GetFrameCount()
// float GetFrameTime()
// float GetFrameRate()


// HRESULT queryCaps();
// HRESULT createDeviceDependentStateInternal();
// queryCaps()

namespace device
{
	inline const char* GetFeatureLevelName(D3D_FEATURE_LEVEL FeatureLevel)
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

	// Global variable declarations here.
	ID3D12Device* g_pDevice{ nullptr };
	ColorBuffer g_DisplayBuffer[3];
	IDXGISwapChain3* g_pDXGISwapChain{ nullptr };

	CommandQueueManager   g_commandQueueManager;
	CommandContextManager g_commandContextManager; // Not Init Yet
	DescriptorHeapManager g_descriptorHeapManager; // Not Init Yet

	D3D_FEATURE_LEVEL g_D3DFeatureLevel;
	D3D12_FEATURE_DATA_D3D12_OPTIONS g_Options;
	D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT g_GpuVaSupport;
	DXGI_QUERY_VIDEO_MEMORY_INFO g_LocalVideoMemoryInfo;
	DXGI_QUERY_VIDEO_MEMORY_INFO g_NonLocalVideoMemoryInfo;

	// IDXGIFactory2* s_pDXGIFactory{ nullptr };
	Microsoft::WRL::ComPtr<IDXGIFactory2> s_pDXGIFactory{ nullptr };

	bool g_bTypedUAVLoadSupport_R11G11B10_FLOAT{ false };
	bool g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT{ false };

	uint32_t g_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	Platform::Agile<Windows::UI::Core::CoreWindow>  g_window;
#endif

	static constexpr UINT SWAP_CHAIN_BUFFER_COUNT{ 3ul };
	UINT g_DisplayBufferCount{ SWAP_CHAIN_BUFFER_COUNT };

	static bool s_DropRandomFrame{ false };
	static bool s_LimitTo30Hz{ false };
	static bool s_EnableVSync{ false };
	static float s_FrameTime = 0.0f;
	static uint64_t s_FrameIndex = 0;
	static int64_t s_FrameStartTick = 0;

	HRESULT Initialize()
	{
		ASSERT(g_pDXGISwapChain == nullptr, "Device Sector has already been intialized");

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

			IDXGIDebug1* pDebug = nullptr;
			ASSERT_HR(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug)));
			{
				pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
				pDebug->Release();
			}

			wprintf(L"Debug layer is %senabled\n\n", bDebugLayerEnabled ? L"" : L"NOT ");
		}
#endif
		ASSERT_HR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&s_pDXGIFactory)));

		createDeviceDependentStateInternal();
		
		premade::Initialize();
		GpuTime::Initialize();
		bufferManager::InitializeAllBuffers(window::g_windowHeight, window::g_windowWidth);
		// TODO : 
		// TextRender Init
		// LDR����, ȭ�� �׸� PSO�� ���̳�, PASS������.

		return S_OK;
	}

	// Try to handle displayBuffer Index.
	HRESULT Resize(uint32_t width, uint32_t height)
	{
		ASSERT(g_pDXGISwapChain != nullptr);

		// Check for invalid window dimensions
		if (width == 0 || height == 0)
		{
			return S_FALSE;
		}

		// Check for an unneeded resize
		if (width == window::g_windowWidth && height == window::g_windowHeight)
		{
			return S_OK;
		}
		g_commandQueueManager.IdleGPU();

		window::g_windowWidth = width;
		window::g_windowHeight = height;

		printf("Changing display resolution to %ux%u", width, height);

		// g_PreDisplayBuffer.Create(L"PreDisplay Buffer", width, height, 1, SwapChainFormat);

		for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			g_DisplayBuffer[i].Destroy();
		}

		ASSERT_HR(g_pDXGISwapChain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, width, height, DXGI_FORMAT_R10G10B10A2_UNORM, 0));

		for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> DisplayPlane;
			ASSERT_HR(g_pDXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&DisplayPlane)));
			g_DisplayBuffer[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
		}

		g_commandQueueManager.IdleGPU();

		bufferManager::DestroyRenderingBuffers();
		bufferManager::InitializeAllBuffers(width, height);

		return S_OK;
	}

	void Terminate()
	{
		g_commandQueueManager.IdleGPU();
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		g_pDXGISwapChain->SetFullscreenState(FALSE, nullptr);
#endif
	}
	void Destroy()
	{
		g_pDXGISwapChain->Release();

		custom::CommandContext::DestroyAllContexts();
		g_commandQueueManager.Shutdown();
		GpuTime::Shutdown();
		
		custom::PSO::DestroyAll();
		custom::RootSignature::DestroyAll();
		custom::DynamicDescriptorHeap::DestroyAll();

		for (size_t i{ 0 }; i < g_DisplayBufferCount; ++i)
		{
			g_DisplayBuffer[i].Destroy();
		}

		premade::Shutdown(); // There is nothing yet.
		bufferManager::DestroyRenderingBuffers();

		g_pDevice->Release();
	}

	void Present()
	{
		uint64_t CurrentTick = SystemTime::GetCurrentTick();

		if (s_EnableVSync)
		{
			// With VSync enabled, the time step between frames becomes a multiple of 16.666 ms.  We need
			// to add logic to vary between 1 and 2 (or 3 fields).  This delta time also determines how
			// long the previous frame should be displayed (i.e. the present interval.)
			s_FrameTime = (s_LimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
			if (s_DropRandomFrame)
			{
				if (std::rand() % 50 == 0)
					s_FrameTime += (1.0f / 60.0f);
			}
		}
		else
		{
			// When running free, keep the most recent total frame time as the time step for
			// the next frame simulation.  This is not super-accurate, but assuming a frame
			// time varies smoothly, it should be close enough.
			s_FrameTime = (float)SystemTime::TimeBetweenTicks(s_FrameStartTick, CurrentTick);
		}

		s_FrameStartTick = CurrentTick;

		++s_FrameIndex;
	}

	HRESULT queryCaps();

	HRESULT createDeviceDependentStateInternal()
	{
		ASSERT(device::g_pDXGISwapChain == nullptr, "Graphics has already been initialized");

		Microsoft::WRL::ComPtr<IDXGIAdapter3> s_pDXGIAdapter;
		{
			// Have the user select the desired graphics adapter.
			uint32_t AdapterOrdinal = 0;

			printf("Available Adapters:\n");
			while (s_pDXGIFactory->EnumAdapters(AdapterOrdinal, ((IDXGIAdapter**)s_pDXGIAdapter.GetAddressOf())) != DXGI_ERROR_NOT_FOUND)
			{
				DXGI_ADAPTER_DESC Desc;
				s_pDXGIAdapter->GetDesc(&Desc);

				printf("  [%d] %ls\n", AdapterOrdinal, Desc.Description);

				s_pDXGIAdapter->Release();
				++AdapterOrdinal;
			}

			while (1)
			{
				printf("\nSelect Adapter: ");

				AdapterOrdinal = 0xFFFFFFFF;
				if (AdapterOrdinal == 0xFFFFFFFF)
				{
					if (scanf_s("%d", &AdapterOrdinal) == 0)
					{
						// If the user did not specify valid input, just use adapter 0.
						AdapterOrdinal = 0;
					}
				}

				ASSERT_HR(s_pDXGIFactory->EnumAdapters(AdapterOrdinal, ((IDXGIAdapter**)s_pDXGIAdapter.GetAddressOf())));

				ASSERT_HR(s_pDXGIAdapter->QueryInterface(s_pDXGIAdapter.GetAddressOf()));
				s_pDXGIAdapter->Release();
				break;
			}
		}
		ASSERT_HR(s_pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &::device::g_LocalVideoMemoryInfo));
		ASSERT_HR(s_pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &::device::g_NonLocalVideoMemoryInfo));

		ASSERT_HR(D3D12CreateDevice((IUnknown*)s_pDXGIAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&::device::g_pDevice)));

		g_commandQueueManager.Create(g_pDevice);

#ifndef RELEASE
		{
			bool DeveloperModeEnabled = false;

			// See Dynamic frequency scaling and Throttling.
			// Look in the Windows Registry to determine if Developer Mode is enabled
			HKEY hKey;
			LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey);
			if (result == ERROR_SUCCESS)
			{
				DWORD keyValue, keySize = sizeof(DWORD);
				result = RegQueryValueEx(hKey, L"AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
				if (result == ERROR_SUCCESS && keyValue == 1)
				{
					DeveloperModeEnabled = true;
				}
				RegCloseKey(hKey);
			}

			ASSERT(DeveloperModeEnabled, "Enable Developer Mode on Windows 10 to get consistent profiling results");

			// Prevent the GPU from overclocking or underclocking to get consistent timings
			if (DeveloperModeEnabled)
			{
				device::g_pDevice->SetStablePowerState(TRUE);
			}
		}
#endif

#if defined(_DEBUG)
		ID3D12InfoQueue* pInfoQueue = nullptr;

		ASSERT_HR(device::g_pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue)));

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
		for (size_t i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			device::g_DescriptorSize[i] = ::device::g_pDevice->GetDescriptorHandleIncrementSize((D3D12_DESCRIPTOR_HEAP_TYPE)i);
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};
		if (SUCCEEDED(device::g_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData))))
		{
			if (FeatureData.TypedUAVLoadAdditionalFormats)
			{
				D3D12_FEATURE_DATA_FORMAT_SUPPORT Support =
				{
					DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
				};

				if (SUCCEEDED(device::g_pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
					(Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
				{
					device::g_bTypedUAVLoadSupport_R11G11B10_FLOAT = true;
				}

				Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

				if (SUCCEEDED(device::g_pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
					(Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
				{
					device::g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
				}
			}
		}

		ASSERT_HR(queryCaps());

		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
		ZeroMemory(&SwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
		SwapChainDesc.Height = window::g_windowHeight;
		SwapChainDesc.Width = window::g_windowWidth;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		SwapChainDesc.BufferCount = device::SWAP_CHAIN_BUFFER_COUNT;
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SwapChainDesc.Scaling = DXGI_SCALING_NONE;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		{
			ID3D12CommandQueue* CommandQueue = g_commandQueueManager.GetCommandQueue();

			ASSERT_HR(s_pDXGIFactory->CreateSwapChainForHwnd(CommandQueue, window::g_hWnd, &SwapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)&g_pDXGISwapChain));
		}
        #else // when running platform is UWP.
        ASSERT(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP), "Invalid Platform.");
        #endif

		for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> DisplayPlane;
			ASSERT_HR(g_pDXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&DisplayPlane)));
			g_DisplayBuffer[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
		}

		return S_OK;
	}

	HRESULT queryCaps()
	{
		ASSERT_HR(g_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &g_Options, sizeof(g_Options)));

		ASSERT_HR(g_pDevice->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &g_GpuVaSupport, sizeof(g_GpuVaSupport)));

		D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevels = {};
		D3D_FEATURE_LEVEL FeatureLevelsList[] = 
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_12_1,
		};
		FeatureLevels.NumFeatureLevels = ARRAYSIZE(FeatureLevelsList);
		FeatureLevels.pFeatureLevelsRequested = FeatureLevelsList;

		ASSERT_HR(::device::g_pDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevels, sizeof(FeatureLevels)));

		printf("\n");
		printf("--- Device details --------------------\n");
		printf("  GPU Virtual Address Info:\n");
		printf("    Max Process Size:      %I64d GB\n", (1ULL << ::device::g_GpuVaSupport.MaxGPUVirtualAddressBitsPerProcess) / 1024 / 1024 / 1024);
		printf("    Max Resource Size:     %I64d GB\n", (1ULL << ::device::g_GpuVaSupport.MaxGPUVirtualAddressBitsPerResource) / 1024 / 1024 / 1024);
		printf("  Feature Level Info:\n");
		printf("    Max Feature Level:     %s\n", device::GetFeatureLevelName(FeatureLevels.MaxSupportedFeatureLevel));
		printf("  Additional Info:\n");
		printf("    Resource Binding Tier: Tier %d\n", ::device::g_Options.ResourceBindingTier);
		printf("    Resource Heap Tier:    Tier %d\n", ::device::g_Options.ResourceHeapTier);
		printf("    Tiled Resource Tier:   Tier %d\n", ::device::g_Options.TiledResourcesTier);
		printf("\n");

		return S_OK;
	}

	uint64_t GetFrameCount()
	{
		return s_FrameIndex;
	}
	float GetFrameTime()
	{
		return s_FrameTime;
	}
	float GetFrameRate()
	{
		return s_FrameTime == 0.0f ? 0.0f : 1.0f / s_FrameTime;
	}
} // end namespace GraphicsDeviceManager