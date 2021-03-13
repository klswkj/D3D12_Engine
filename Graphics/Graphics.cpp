#include "stdafx.h"
#include "Graphics.h"
#include "Device.h"
#include "window.h"
#include "RootSignature.h"
#include "CommandContext.h"
#include "ComputeContext.h"
#include "CommandSignature.h"
#include "CommandQueueManager.h"
#include "DescriptorHeapManager.h"
#include "PSO.h"
#include "BufferManager.h"
#include "PreMadePSO.h"
#include "TextureManager.h"
#include "TextManager.h"
#include "GpuTime.h"
#include "CpuTime.h"

#pragma region SHADER_HEADER
#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/ScreenQuadVS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/BufferCopyPS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/PresentHDRPS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/PresentSDRPS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/MagnifyPixelsPS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/BicubicHorizontalUpsamplePS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/BilinearUpsamplePS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/BicubicVerticalUpsamplePS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/SharpeningUpsamplePS.h"

#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GenerateMipsLinearCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GenerateMipsLinearOddXCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GenerateMipsLinearOddYCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GenerateMipsLinearOddCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GenerateMipsGammaCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GenerateMipsGammaOddXCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GenerateMipsGammaOddYCS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GenerateMipsGammaOddCS.h"

#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/DebugCS.h"

#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/ScreenQuadVS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/BufferCopyPS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/PresentHDRPS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/PresentSDRPS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/MagnifyPixelsPS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/BicubicHorizontalUpsamplePS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/BilinearUpsamplePS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/BicubicVerticalUpsamplePS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/SharpeningUpsamplePS.h"

#include "../x64/Release/Graphics(.lib)/CompiledShaders/GenerateMipsLinearCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/GenerateMipsLinearOddXCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/GenerateMipsLinearOddYCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/GenerateMipsLinearOddCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/GenerateMipsGammaCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/GenerateMipsGammaOddXCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/GenerateMipsGammaOddYCS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/GenerateMipsGammaOddCS.h"
#endif
#pragma endregion

namespace graphics
{
    custom::RootSignature s_PresentRS;
    custom::RootSignature s_PresentHDR_RS;

    GraphicsPSO s_BlendUIPSO;
    GraphicsPSO s_PresentSDR_PSO;
    GraphicsPSO s_PresentHDR_PSO;
    GraphicsPSO s_MagnifyPixelsPSO;
    GraphicsPSO s_SharpeningUpsamplePSO;
    GraphicsPSO s_BicubicHorizontalUpsamplePSO;
    GraphicsPSO s_BicubicVerticalUpsamplePSO;
    GraphicsPSO s_BilinearUpsamplePSO;

    custom::RootSignature g_GenerateMipsRootSignature;
    custom::RootSignature g_PresentRootSignature;
    custom::RootSignature g_CopyRootSignature;
	DescriptorHeapManager g_DescriptorHeapManager;

    ComputePSO g_GenerateMipsLinearComputePSO[4];
    ComputePSO g_GenerateMipsGammaComputePSO[4];
    ComputePSO g_CopyPSO;

    custom::CommandSignature g_DispatchIndirectCommandSignature(1); // goto ReadyMadePSO
    custom::CommandSignature g_DrawIndirectCommandSignature(1);     // goto ReadyMadePSO

    uint64_t g_CurrentBufferIndex = 0ull;

    struct SwapChainContext
    {
        uint64_t     FenceValue;
        HANDLE       FenceEvent;
        ID3D12Fence* LastSubmitToSwapChain;
    };
    SwapChainContext g_SwapChainContexts[3]; // device::g_DisplayBufferCount

    bool     g_bHDROutput          = false;
    float    g_HDRPaperWhite       = 150.0f;
    float    g_MaxDisplayLuminance = 1000.0f;
    uint32_t g_bHDRDebugMode       = 0u;

    enum class UpsampleFilter
    {
        kBicubic = 0,
        kBilinear,
        kSharpening
    };
    UpsampleFilter g_UpsampleFilter = UpsampleFilter::kSharpening;
    float g_BicubicUpsampleWeight   = -0.75f;
    float g_SharpeningRotation      = 45.0f;
    float g_SharpeningSpread        = 1.0f;
    float g_SharpeningStrength      = 0.1f;

    void initializePresent();
}

namespace
{
    static bool s_DropRandomFrame = false;
    static bool s_LimitTo30Hz = false;
    static bool s_EnableVSync = false;
    static float s_FrameTime = 0.0f;
    static uint64_t s_FrameIndex = 0;
    static int64_t s_FrameStartTick = 0;

    static float s_DebugFrameTime = 0;
    static uint64_t s_DebugFrameStartTick = 0;
}

void graphics::Initialize()
{
    TextureManager::Initialize();
    TextManager::Initialize();
    initializePresent();
    // GraphRenderer::Initialize();

    GPUTime::ClockInitialize();
    CPUTime::Initialize();
}

void graphics::initializePresent()
{
#if defined(NTDDI_WIN10_RS3) && (NTDDI_WIN10_RS3 <= NTDDI_VERSION)
    {
        IDXGISwapChain4* swapChain = (IDXGISwapChain4*)device::g_pDXGISwapChain;
        Microsoft::WRL::ComPtr<IDXGIOutput> output;
        Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
        DXGI_OUTPUT_DESC1 outputDesc;
        UINT colorSpaceSupport;
        
        // Query support for ST.2084 on the display and set the color space accordingly
        if (SUCCEEDED(swapChain->GetContainingOutput(&output)) &&
            SUCCEEDED(output.As(&output6)) &&
            SUCCEEDED(output6->GetDesc1(&outputDesc)) &&
            outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 &&
            SUCCEEDED(swapChain->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport)) &&
            (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
            SUCCEEDED(swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
        {
            g_bHDROutput = true;
        }
        
    }
#else
#ifdef _DEBUG
    printf("Need to Window Update over RS3.\nFailed to Enable HDROutput.\n");
#endif
#endif

    // s_PresentRS.Reset(4, 2);
    s_PresentRS.Reset(2, 2);
    s_PresentRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    s_PresentRS[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
    // s_PresentRS[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    // s_PresentRS[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1); // Not set
    s_PresentRS.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
    s_PresentRS.InitStaticSampler(1, premade::g_SamplerPointClampDesc);
    s_PresentRS.Finalize(L"s_Present_RS");

    // s_PresentHDR_RS.Reset(3, 2);
    s_PresentHDR_RS.Reset(2, 2);
    s_PresentHDR_RS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    s_PresentHDR_RS[1].InitAsConstants(0, 5, D3D12_SHADER_VISIBILITY_ALL);
    // s_PresentHDR_RS[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    s_PresentHDR_RS.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
    s_PresentHDR_RS.InitStaticSampler(1, premade::g_SamplerPointClampDesc);
    s_PresentHDR_RS.Finalize(L"s_PresentHDR_RS");

    // Initialize PSOs
    s_BlendUIPSO.SetRootSignature        (s_PresentRS);
    s_BlendUIPSO.SetRasterizerState      (premade::g_RasterizerTwoSided);
    s_BlendUIPSO.SetBlendState           (premade::g_BlendPreMultiplied);
    s_BlendUIPSO.SetDepthStencilState    (premade::g_DepthStateDisabled);
    s_BlendUIPSO.SetSampleMask           (0xFFFFFFFF);
    s_BlendUIPSO.SetInputLayout          (0, nullptr);
    s_BlendUIPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    s_BlendUIPSO.SetVertexShader         (g_pScreenQuadVS, sizeof(g_pScreenQuadVS));
    s_BlendUIPSO.SetPixelShader          (g_pBufferCopyPS, sizeof(g_pBufferCopyPS));
    s_BlendUIPSO.SetRenderTargetFormat   (device::SwapChainFormat, DXGI_FORMAT_UNKNOWN);
    s_BlendUIPSO.Finalize(L"BlendUIPSO");

#define CreatePSO( ObjName, ShaderByteCode )                         \
    ObjName = s_BlendUIPSO;                                          \
    ObjName.SetBlendState(premade::g_BlendDisable);                  \
    ObjName.SetPixelShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize(L#ObjName);

    CreatePSO(s_PresentSDR_PSO,               g_pPresentSDRPS);
    CreatePSO(s_MagnifyPixelsPSO,             g_pMagnifyPixelsPS);
    //CreatePSO(s_BilinearUpsamplePSO,          g_pBilinearUpsamplePS);
    //CreatePSO(s_BicubicHorizontalUpsamplePSO, g_pBicubicHorizontalUpsamplePS);
    //CreatePSO(s_BicubicVerticalUpsamplePSO,   g_pBicubicVerticalUpsamplePS);
    //CreatePSO(s_SharpeningUpsamplePSO,        g_pSharpeningUpsamplePS);
    
    s_SharpeningUpsamplePSO.SetRootSignature(s_PresentRS);
    s_SharpeningUpsamplePSO.SetRasterizerState(premade::g_RasterizerTwoSided);
    s_SharpeningUpsamplePSO.SetBlendState(premade::g_BlendDisable);
    s_SharpeningUpsamplePSO.SetDepthStencilState(premade::g_DepthStateDisabled);
    s_SharpeningUpsamplePSO.SetSampleMask(0xFFFFFFFF);
    s_SharpeningUpsamplePSO.SetInputLayout(0, nullptr);
    s_SharpeningUpsamplePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    s_SharpeningUpsamplePSO.SetVertexShader(g_pScreenQuadVS, sizeof(g_pScreenQuadVS));
    s_SharpeningUpsamplePSO.SetPixelShader(g_pSharpeningUpsamplePS, sizeof(g_pSharpeningUpsamplePS));
    s_SharpeningUpsamplePSO.SetRenderTargetFormat(DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN);
    s_SharpeningUpsamplePSO.Finalize(L"s_SharpeningUpsamplePSO");

#undef CreatePSO
    // BicubicUpsamplePS
    s_BicubicHorizontalUpsamplePSO = s_BlendUIPSO;
    s_BicubicHorizontalUpsamplePSO.SetBlendState(premade::g_BlendDisable);
    s_BicubicHorizontalUpsamplePSO.SetPixelShader(g_pBicubicHorizontalUpsamplePS, sizeof(g_pBicubicHorizontalUpsamplePS));
    s_BicubicHorizontalUpsamplePSO.SetRenderTargetFormat(DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_UNKNOWN);
    s_BicubicHorizontalUpsamplePSO.Finalize(L"BicubicHorizontalUpsamplePSO");

    s_PresentHDR_PSO = s_PresentSDR_PSO;
    s_PresentHDR_PSO.SetRootSignature(s_PresentHDR_RS);
    s_PresentHDR_PSO.SetPixelShader(g_pPresentHDRPS, sizeof(g_pPresentHDRPS));

    DXGI_FORMAT SwapChainFormats[3] = { device::SwapChainFormat, device::SwapChainFormat, device::SwapChainFormat };

    s_PresentHDR_PSO.SetRenderTargetFormats(_countof(SwapChainFormats), SwapChainFormats, DXGI_FORMAT_UNKNOWN);
    s_PresentHDR_PSO.Finalize(L"Present HDR PSO");

    g_GenerateMipsRootSignature.Reset(3, 1);
    g_GenerateMipsRootSignature[0].InitAsConstants(0, 4);
    g_GenerateMipsRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    g_GenerateMipsRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4);
    g_GenerateMipsRootSignature.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
    g_GenerateMipsRootSignature.Finalize(L"Generate Mips PSO");

#define CreatePSO(ObjName, ShaderByteCode )                            \
    ObjName.SetRootSignature(g_GenerateMipsRootSignature);             \
    ObjName.SetComputeShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize(L#ObjName);

    CreatePSO(g_GenerateMipsLinearComputePSO[0], g_pGenerateMipsLinearCS);
    CreatePSO(g_GenerateMipsLinearComputePSO[1], g_pGenerateMipsLinearOddXCS);
    CreatePSO(g_GenerateMipsLinearComputePSO[2], g_pGenerateMipsLinearOddYCS);
    CreatePSO(g_GenerateMipsLinearComputePSO[3], g_pGenerateMipsLinearOddCS);
    CreatePSO(g_GenerateMipsGammaComputePSO[0],  g_pGenerateMipsGammaCS);
    CreatePSO(g_GenerateMipsGammaComputePSO[1],  g_pGenerateMipsGammaOddXCS);
    CreatePSO(g_GenerateMipsGammaComputePSO[2],  g_pGenerateMipsGammaOddYCS);
    CreatePSO(g_GenerateMipsGammaComputePSO[3],  g_pGenerateMipsGammaOddCS);
#undef CreatePSO

    // Not Used.
    {
        {
            /*
            g_CopyRootSignature.Reset(2, 2);
            g_CopyRootSignature.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
            g_CopyRootSignature.InitStaticSampler(1, premade::g_SamplerLinearBorderDesc);
            g_CopyRootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
            g_CopyRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
            g_CopyRootSignature.Finalize(L"Graphics Debug Compute RS");
            */
        }
        {
            g_CopyRootSignature.Reset(1, 2);
            g_CopyRootSignature.InitStaticSampler(0, premade::g_SamplerLinearClampDesc);
            g_CopyRootSignature.InitStaticSampler(1, premade::g_SamplerLinearBorderDesc);
            g_CopyRootSignature[0].InitAsDescriptorTable(2);
            g_CopyRootSignature[0].SetTableRange(0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
            g_CopyRootSignature[0].SetTableRange(1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
            g_CopyRootSignature.Finalize(L"Graphics Debug Compute RS");
        }
        g_CopyPSO.SetRootSignature(g_CopyRootSignature);
        g_CopyPSO.SetComputeShader(g_pDebugCS, sizeof(g_pDebugCS));
        g_CopyPSO.Finalize(L"Graphics Debug Compute PSO");
    }
}

void graphics::ShutDown()
{
    TextureManager::Shutdown();
    TextManager::Shutdown();
    // TODO : GraphRenderer::Shutdown(); - Not Used yet.

    GPUTime::Shutdown();
    // CPUTime::Shutdown(); - Not Used yet.
}

uint64_t PresentHDR()
{
    custom::GraphicsContext& Context = custom::GraphicsContext::Begin(1);

    Context.PIXBeginEvent(L"Present HDR", 0u);

    // We're going to be reading these buffers to write to the swap chain buffer(s)
    Context.TransitionResource(bufferManager::g_OverlayBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SubmitResourceBarriers(0u);
    Context.SetRenderTarget(bufferManager::g_OverlayBuffer.GetRTV(), 0u);
    Context.ClearColor(bufferManager::g_OverlayBuffer, 0u);
    Context.SetViewportAndScissor(0, 0, bufferManager::g_OverlayBuffer.GetWidth(), bufferManager::g_OverlayBuffer.GetHeight(), 0u);

    Context.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(bufferManager::g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(device::g_DisplayColorBuffer[graphics::g_CurrentBufferIndex % device::g_DisplayBufferCount], D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SubmitResourceBarriers(0u);

    LONG Width = window::g_TargetWindowWidth;
    LONG Height = window::g_TargetWindowHeight;

    D3D12_CPU_DESCRIPTOR_HANDLE RTVs[] =
    {
        device::g_DisplayColorBuffer[graphics::g_CurrentBufferIndex % device::g_DisplayBufferCount].GetRTV()
    };

    Context.SetRootSignature(graphics::s_PresentHDR_RS, 0u);
    Context.SetPipelineState(graphics::s_PresentHDR_PSO, 0u);

    Context.SetDynamicDescriptor(0, 0, bufferManager::g_SceneColorBuffer.GetSRV(), 0u);
    Context.SetDynamicDescriptor(0, 1, bufferManager::g_OverlayBuffer.GetSRV(), 0u);

    Context.SetRenderTargets(_countof(RTVs), RTVs, 0u);
    Context.SetViewportAndScissor(0, 0, Width, Height, 0u);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    struct Constants
    {
        float RcpDstWidth;
        float RcpDstHeight;
        float PaperWhite;
        float MaxBrightness; // 1,000 Candela per meter square
        uint32_t DebugMode;   // position.x * RcpDstSize.x < 0.5 ? HDR : SDR
    };

    Constants consts =
    {
        1.0f / Width,
        1.0f / Height,
        graphics::g_HDRPaperWhite,
        graphics::g_MaxDisplayLuminance,
        graphics::g_bHDRDebugMode
    };

    Context.SetConstantArray(1, sizeof(Constants) / 4, (float*)&consts, 0u);
    Context.Draw(3, 0U, 0u);

    Context.TransitionResource(device::g_DisplayColorBuffer[graphics::g_CurrentBufferIndex % device::g_DisplayBufferCount], D3D12_RESOURCE_STATE_PRESENT);
    Context.SubmitResourceBarriers(0u);
    Context.PIXEndEvent(0u);

    // Close the final context to be executed before frame present.
    return Context.Finish(false);
}

void CompositeOverlays(custom::GraphicsContext& Context, const uint8_t commandIndex)
{
    // Blend (or write) the UI overlay
    Context.PIXBeginEvent(L"CompositeOverLays", commandIndex);

    Context.TransitionResource(bufferManager::g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SubmitResourceBarriers(commandIndex);

    Context.SetPipelineState(graphics::s_BlendUIPSO, commandIndex);

    Context.SetDynamicDescriptor(0, 0, bufferManager::g_OverlayBuffer.GetSRV(), commandIndex);
    Context.SetConstants(1, 1.0f / window::g_TargetWindowWidth, 1.0f / window::g_TargetWindowHeight, commandIndex);
    Context.Draw(3U, 0U, commandIndex);

    Context.PIXEndEvent(commandIndex);
}

void PresentLDR()
{
    custom::GraphicsContext& Context = custom::GraphicsContext::Begin(1);
    Context.PIXBeginEvent(L"Present LDR", 0u);

    Context.SetRootSignature(graphics::s_PresentRS, 0u);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, true);

    // Copy (and convert) the LDR buffer to the back buffer
    Context.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SubmitResourceBarriers(0u);
    Context.SetDynamicDescriptor(0U, 0U, bufferManager::g_SceneColorBuffer.GetSRV(), 0u);
    ColorBuffer& UpsampleDest = device::g_DisplayColorBuffer[graphics::g_CurrentBufferIndex % device::g_DisplayBufferCount];

    if (window::g_TargetWindowWidth == device::g_DisplayWidth && window::g_TargetWindowHeight == device::g_DisplayHeight)
    {
        Context.PIXBeginEvent(L"Present Normal", 0u);
        Context.SetPipelineState(graphics::s_PresentSDR_PSO, 0u);
        Context.TransitionResource(UpsampleDest, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SubmitResourceBarriers(0u);
        Context.SetRenderTarget(UpsampleDest.GetRTV(), 0u);
        Context.SetViewportAndScissor(0L, 0L, (LONG)device::g_DisplayWidth, (LONG)device::g_DisplayHeight, 0u);
        Context.Draw(3U, 0U, 0u);
        Context.PIXEndEvent(0u);
    }
    else if (graphics::g_UpsampleFilter == graphics::UpsampleFilter::kBicubic)
    {
        Context.PIXBeginEvent(L"Bicubic Filter", 0u);
        Context.TransitionResource(bufferManager::g_HorizontalBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SubmitResourceBarriers(0u);
        Context.SetRenderTarget(bufferManager::g_HorizontalBuffer.GetRTV(), 0u);
        Context.SetViewportAndScissor(0U, 0U, (LONG)device::g_DisplayWidth, (LONG)window::g_TargetWindowHeight, 0u);
        Context.SetPipelineState(graphics::s_BicubicHorizontalUpsamplePSO, 0u);
        Context.SetConstants(1U, (float)window::g_TargetWindowWidth, (float)window::g_TargetWindowHeight, (float)graphics::g_BicubicUpsampleWeight, 0u);
        Context.Draw(3U, 0U, 0u);

        Context.TransitionResource(bufferManager::g_HorizontalBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.TransitionResource(UpsampleDest, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SubmitResourceBarriers(0u);
        Context.SetRenderTarget(UpsampleDest.GetRTV(), 0u);
        Context.ClearColor(UpsampleDest, 0u);
        Context.SetViewportAndScissor(0L, 0L, (LONG)device::g_DisplayWidth, (LONG)device::g_DisplayHeight, 0u);
        Context.SetPipelineState(graphics::s_BicubicVerticalUpsamplePSO, 0u);
        Context.SetDynamicDescriptor(0U, 0U, bufferManager::g_HorizontalBuffer.GetSRV(), 0u);
        Context.SetConstants(1U, (float)device::g_DisplayWidth, (float)window::g_TargetWindowHeight, (float)graphics::g_BicubicUpsampleWeight, 0u);
        Context.Draw(3U, 0U, 0u);
        Context.PIXEndEvent(0u);
    }
    else if (graphics::g_UpsampleFilter == graphics::UpsampleFilter::kSharpening)
    {
        Context.PIXBeginEvent(L"Sharpening Filter", 0u);
        Context.SetPipelineState(graphics::s_SharpeningUpsamplePSO, 0u);
        Context.TransitionResource(UpsampleDest, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SubmitResourceBarriers(0u);
        Context.SetRenderTarget(UpsampleDest.GetRTV(), 0u);
        Context.ClearColor(UpsampleDest, 0u);
        Context.SetViewportAndScissor(0L, 0L, (LONG)device::g_DisplayWidth, (LONG)device::g_DisplayHeight, 0u);
        float TexelWidth  = 1.0f / window::g_TargetWindowWidth;
        float TexelHeight = 1.0f / window::g_TargetWindowHeight;
        float X = std::cos(graphics::g_SharpeningRotation / 180.0f * 3.14159f) * graphics::g_SharpeningSpread;
        float Y = std::sin(graphics::g_SharpeningRotation / 180.0f * 3.14159f) * graphics::g_SharpeningSpread;
        const float WA = graphics::g_SharpeningStrength;
        const float WB = 1.0f + 4.0f * WA;
        float Constants[6] = { X * TexelWidth, Y * TexelHeight, Y * TexelWidth, -X * TexelHeight, WA, WB };
        Context.SetConstantArray(1U, (UINT)_countof(Constants), (void*)Constants, 0u);
        // Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context.Draw(3U, 0U, 0u);
        Context.PIXEndEvent(0u);
    }
    else if (graphics::g_UpsampleFilter == graphics::UpsampleFilter::kBilinear)
    {
        Context.PIXBeginEvent(L"Bilinear Filter", 0u);
        Context.SetPipelineState(graphics::s_BilinearUpsamplePSO, 0u);
        Context.TransitionResource(UpsampleDest, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SubmitResourceBarriers(0u);
        Context.SetRenderTarget(UpsampleDest.GetRTV(), 0u);
        Context.SetViewportAndScissor(0L, 0L, (LONG)device::g_DisplayWidth, (LONG)device::g_DisplayHeight, 0u);
        Context.Draw(3U, 0U, 0u);
        Context.PIXEndEvent(0u);
    }

    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CompositeOverlays(Context, 0u);

    Context.TransitionResource(device::g_DisplayColorBuffer[graphics::g_CurrentBufferIndex % device::g_DisplayBufferCount], D3D12_RESOURCE_STATE_PRESENT);
    Context.TransitionResource(bufferManager::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SubmitResourceBarriers(0u);

    Context.PIXEndEvent(0u);

    // Close the final context to be executed before frame present.
    Context.Finish(false);
}

void graphics::Present()
{
    uint64_t CurrentTick = CPUTime::GetCurrentTick();

    if (g_bHDROutput)
    {
        // PresentHDR();
    }
    else
    {
        PresentLDR();
        /*
        ColorBuffer& Dest = device::g_DisplayColorBuffer[graphics::g_CurrentBufferIndex % device::g_DisplayBufferCount];
        ColorBuffer& Src = bufferManager::g_SceneColorBuffer;
        custom::ComputeContext& CC = custom::ComputeContext::Begin(L"Testing");
        CC.SetRootSignature(g_CopyRootSignature);
        CC.SetPipelineState(g_CopyPSO);
        CC.SetDynamicDescriptor(0, 0, Dest.GetUAV());
        CC.SetDynamicDescriptor(0, 1, Src.GetSRV());
        CC.InsertUAVBarrier(Dest, true);
        CC.TransitionResource(Dest, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
        CC.TransitionResource(Src, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
        CC.Dispatch2D(Src.GetWidth(), Src.GetHeight());
        CC.Finish();
        */
    }

    g_CurrentBufferIndex = (g_CurrentBufferIndex + 1);

    // HANDLE waitableObjests[2] = { device::g_hSwapChainWaitableObject, nullptr };

#undef max
    UINT PresentInterval = s_EnableVSync ? std::max(4, (int)Math::Round(s_FrameTime * 60.0f)) : 0;

    static HRESULT swapchainResult;

    ASSERT_HR(swapchainResult = device::g_pDXGISwapChain->Present(PresentInterval, 0));
    if (FAILED(swapchainResult))
    {
        ID3D12Device* pTempDevice;

        device::g_pDXGISwapChain->GetDevice(IID_PPV_ARGS(&pTempDevice));

        swapchainResult = pTempDevice->GetDeviceRemovedReason();

        SafeRelease(pTempDevice);
        ASSERT_HR(swapchainResult);
    }

    if (s_EnableVSync)
    {
        s_FrameTime = (s_LimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
        if (s_DropRandomFrame)
        {
            if (std::rand() % 60 == 0)
			{
				s_FrameTime += (1.0f / 60.0f);
			}
        }
    }
    else
    {
        s_FrameTime = (float)CPUTime::TimeBetweenTicks(s_FrameStartTick, CurrentTick);
    }

    s_FrameStartTick = CurrentTick;

    ++s_FrameIndex;
}

void graphics::SetFrameCountZero()
{
    s_FrameIndex = 0ull;
}

uint64_t graphics::GetFrameCount()
{
    return s_FrameIndex;
}
float graphics::GetFrameTime()
{
    return s_FrameTime;
}
float graphics::GetFrameRate()
{
    return s_FrameTime == 0.0f ? 0.0f : 1.0f / s_FrameTime;
}

void graphics::InitDebugStartTick()
{
    s_DebugFrameStartTick = CPUTime::GetCurrentTick();
}
float graphics::GetDebugFrameTime()
{
    uint64_t DebugCurrentTick = CPUTime::GetCurrentTick();
    s_DebugFrameTime = (float)CPUTime::TimeBetweenTicks(s_DebugFrameStartTick, DebugCurrentTick);
    s_DebugFrameStartTick = DebugCurrentTick;

    return s_DebugFrameTime;
}