#include "stdafx.h"
#include "CustomD3D12Adapter.h"

// Returns bool whether the device supports DirectX Raytracing tier.
inline bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter)
{
    Microsoft::WRL::ComPtr<ID3D12Device> testDevice;
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

    return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
        && SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
        && featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

namespace custom
{
    HRESULT D3D12Adapter::InitializeD3D12Adapter()
    {
        IDXGIAdapter* pTempAdapter = nullptr;

        ASSERT_HR(m_pParentFactory->EnumAdapters(m_AdapterIndexInPhysicalMachine, (IDXGIAdapter**)&m_pDXGIAdapter3));
        ASSERT_HR(::D3D12CreateDevice((IUnknown*)m_pDXGIAdapter3, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pRootDevice3)));
        
        // With D3D12MemoryManagement

    }
    void D3D12Adapter::DestoryD3D12Adapter()
    {
        ASSERT(false);
    }

    ::DXGI_QUERY_VIDEO_MEMORY_INFO D3D12Adapter::GetLocalVideoMemoryInfo()
    {
        ASSERT(false);
    }

    void D3D12Adapter::EnqueueD3D12Object(ID3D12Object* pObject, uint64_t fenceValue)
    {
        ASSERT(pObject && fenceValue);

        ASSERT(false);
    }

    void D3D12Adapter::ReleaseD3D12Object(uint64_t fenceValue)
    {
        ASSERT(false);
    }
}