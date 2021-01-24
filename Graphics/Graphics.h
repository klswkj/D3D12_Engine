#pragma once
#include "DescriptorHeapManager.h"
namespace custom
{
    class RootSignature;
    class CommandSignature;
}

class ComputePSO;

namespace graphics
{
    void Initialize();
    // void Resize(uint32_t Width, uint32_t Height);
    void Present();
    void ShutDown();

    uint64_t GetFrameCount();
    float GetFrameTime();
    float GetFrameRate();

    void InitDebugStartTick();
    float GetDebugFrameTime();

    extern custom::RootSignature g_GenerateMipsRootSignature;
    // extern custom::RootSignature g_PresentRootSignature;
    extern ComputePSO            g_GenerateMipsLinearComputePSO[4];
    extern ComputePSO            g_GenerateMipsGammaComputePSO[4];
    
    extern DescriptorHeapManager g_DescriptorHeapManager;

    inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
    {
        return g_DescriptorHeapManager.Allocate(Type, Count);
    }

    extern custom::CommandSignature g_DispatchIndirectCommandSignature;
    extern custom::CommandSignature g_DrawIndirectCommandSignature;

    extern uint64_t  g_CurrentBufferIndex;
    extern float     g_HDRPaperWhite;
    extern float     g_MaxDisplayLuminance;
    extern uint32_t  g_bHDRDebugMode;

    enum class UpsampleFilter;
    extern UpsampleFilter g_UpsampleFilter;
    extern float g_BicubicUpsampleWeight;
    extern float g_SharpeningRotation;
    extern float g_SharpeningSpread;
    extern float g_SharpeningStrength;
}