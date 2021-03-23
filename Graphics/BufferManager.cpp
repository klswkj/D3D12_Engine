#include "stdafx.h"

#include "BufferManager.h"
#include "CommandContext.h"

#include "Window.h"
#include "Graphics.h"
#include "Color.h"

namespace bufferManager
{
     DepthBuffer g_SceneDepthBuffer;    // D32_FLOAT_S8_UINT
     ColorBuffer g_SceneColorBuffer;    // R11G11B10_FLOAT
     ColorBuffer g_PostEffectsBuffer;   // R32_UINT (to support Read-Modify-Write with a UAV)
     ColorBuffer g_OverlayBuffer;       // R8G8B8A8_UNORM
     ColorBuffer g_HorizontalBuffer;    // For separable (bicubic) upsampling

     ColorBuffer  g_VelocityBuffer;     // R10G10B10  (3D velocity)
     ShadowBuffer g_ShadowBuffer;

     ColorBuffer g_SceneDebugBuffer(custom::Color(0.5f, 0.5f, 0.5f));
     ColorBuffer g_OutlineBuffer;
     ColorBuffer g_OutlineHelpBuffer;

     ColorBuffer g_SSAOFullScreen(custom::Color(1.0f, 1.0f, 1.0f));    // R8_UNORM
     ColorBuffer g_LinearDepth[3];    // Normalized planar distance (0 at eye, 1 at far plane) computed from the SceneDepthBuffer
     ColorBuffer g_MinMaxDepth8;      // Min and max depth values of 8x8 tiles
     ColorBuffer g_MinMaxDepth16;     // Min and max depth values of 16x16 tiles
     ColorBuffer g_MinMaxDepth32;     // Min and max depth values of 16x16 tiles
     ColorBuffer g_DepthDownsize1;
     ColorBuffer g_DepthDownsize2;
     ColorBuffer g_DepthDownsize3;
     ColorBuffer g_DepthDownsize4;
     ColorBuffer g_DepthTiled1;
     ColorBuffer g_DepthTiled2;
     ColorBuffer g_DepthTiled3;
     ColorBuffer g_DepthTiled4;
     ColorBuffer g_AOMerged1;
     ColorBuffer g_AOMerged2;
     ColorBuffer g_AOMerged3;
     ColorBuffer g_AOMerged4;
     ColorBuffer g_AOSmooth1;
     ColorBuffer g_AOSmooth2;
     ColorBuffer g_AOSmooth3;
     ColorBuffer g_AOHighQuality1;
     ColorBuffer g_AOHighQuality2;
     ColorBuffer g_AOHighQuality3;
     ColorBuffer g_AOHighQuality4;

     ColorBuffer g_DoFTileClass[3];
     ColorBuffer g_DoFPresortBuffer;
     ColorBuffer g_DoFPrefilter;
     ColorBuffer g_DoFBlurColor[3];
     ColorBuffer g_DoFBlurAlpha[3];
     custom::StructuredBuffer g_DoFWorkQueue;
     custom::StructuredBuffer g_DoFFastQueue;
     custom::StructuredBuffer g_DoFFixupQueue;

     ColorBuffer g_MotionPrepBuffer;        // R10G10B10A2
     ColorBuffer g_LumaBuffer;
     ColorBuffer g_TemporalColor[3];

     ColorBuffer g_aBloomUAV1[3];        // 640x384 (1/3)
     ColorBuffer g_aBloomUAV2[3];        // 320x192 (1/6)  
     ColorBuffer g_aBloomUAV3[3];        // 160x96  (1/12)
     ColorBuffer g_aBloomUAV4[3];        // 80x48   (1/24)
     ColorBuffer g_aBloomUAV5[3];        // 40x24   (1/48)
     ColorBuffer g_LumaLR;

     custom::ByteAddressBuffer g_Histogram;
     custom::ByteAddressBuffer g_FXAAWorkCounters;
     custom::ByteAddressBuffer g_FXAAWorkQueue;
     custom::TypedBuffer g_FXAAColorQueue(DXGI_FORMAT_R11G11B10_FLOAT);

     // For testing GenerateMipMaps()
     ColorBuffer g_GenMipsBuffer;

     std::vector<LightData>     g_Lights;
     std::vector<Math::Matrix4> g_LightShadowMatrixes;

     custom::StructuredBuffer   g_LightBuffer;      // lightBuffer         : register(t66);
     ColorBuffer                g_LightShadowArray; // lightShadowArrayTex : register(t67);
     ShadowBuffer               g_CumulativeShadowBuffer;

     custom::ByteAddressBuffer  g_LightGrid;        // lightGrid           : register(t68);
     custom::ByteAddressBuffer  g_LightGridBitMask; // lightGridBitMask    : register(t69);
     constexpr static DXGI_FORMAT DefaultColorFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
     // constexpr static DXGI_FORMAT DefaultColorFormat = DXGI_FORMAT_R11G11B10_FLOAT;
     constexpr static DXGI_FORMAT DefaultDepthFormat = DXGI_FORMAT_D32_FLOAT;
     constexpr static DXGI_FORMAT DefaultDepthStencilFormat  = DXGI_FORMAT_D24_UNORM_S8_UINT;
     constexpr static DXGI_FORMAT DefaultOutlineBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
}

void bufferManager::InitializeAllBuffers(const uint32_t width, const uint32_t height)
{
    g_FXAAWorkCounters.CreateUAVCommitted(L"FXAA Work Counters", 2, sizeof(uint32_t));

    custom::GraphicsContext& graphicsContext = custom::GraphicsContext::Begin(1);
    graphicsContext.ClearUAV(g_FXAAWorkCounters, 0);
    graphicsContext.Finish(false);

    uint32_t startingPoint[14] = {};

    uint32_t* bufferWidth = startingPoint;
    uint32_t* bufferHeight = (uint32_t*)startingPoint + 7;

    // bufferWidth = { width, width / 2, width / 4, width / 8, width / 16, width / 32, width / 64 };
    // bufferHeight = { height, height / 2, height / 4, height / 8, height / 16, height / 32, height / 64 };
    bufferWidth[0] = width;
    bufferHeight[0] = height;

    for (size_t i = 1; i < 7; ++i)
    {
        bufferWidth[i] = bufferWidth[i - 1] >> 1;
    }
    
    for (size_t i = 1; i < 7; ++i)
    {
        bufferHeight[i] = bufferHeight[i - 1] >> 1;
    }

    g_SceneColorBuffer.CreateCommitted(L"Main Color Buffer", bufferWidth[0], bufferHeight[0], 1, DefaultColorFormat, true);
    g_VelocityBuffer.CreateCommitted(L"Motion Vectors", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R32_UINT);
    g_PostEffectsBuffer.CreateCommitted(L"Post Effects Buffer", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R32_UINT);

    g_LinearDepth[0].CreateCommitted(L"Linear Depth 0", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16_UNORM);
    g_LinearDepth[1].CreateCommitted(L"Linear Depth 1", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16_UNORM);
    g_LinearDepth[2].CreateCommitted(L"Linear Depth 2", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16_UNORM);
    g_MinMaxDepth8.CreateCommitted(L"MinMaxDepth 8x8", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R32_UINT);
    g_MinMaxDepth16.CreateCommitted(L"MinMaxDepth 16x16", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R32_UINT);
    g_MinMaxDepth32.CreateCommitted(L"MinMaxDepth 32x32", bufferWidth[5], bufferHeight[5], 1, DXGI_FORMAT_R32_UINT);

    // g_SceneDepthBuffer.Create(L"Scene Depth Buffer", bufferWidth[0], bufferHeight[0], DefaultDepthFormat);
    g_SceneDepthBuffer.Create(L"Scene Depth-Stencil Buffer", bufferWidth[0], bufferHeight[0], DefaultDepthStencilFormat);
    g_OutlineBuffer.CreateCommitted(L"Outline Buffer", bufferWidth[1], bufferHeight[1], 1, DefaultOutlineBufferFormat, true);
    g_OutlineHelpBuffer.CreateCommitted(L"OutlineHelp Buffer", bufferWidth[1], bufferHeight[1], 1, DefaultOutlineBufferFormat);

    g_SSAOFullScreen.CreateCommitted(L"SSAO Full Resolution", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R8_UNORM);

    g_DepthDownsize1.CreateCommitted(L"Depth Down-Sized 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R32_FLOAT);
    g_DepthDownsize2.CreateCommitted(L"Depth Down-Sized 2", bufferWidth[2], bufferHeight[2], 1, DXGI_FORMAT_R32_FLOAT);
    g_DepthDownsize3.CreateCommitted(L"Depth Down-Sized 3", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R32_FLOAT);
    g_DepthDownsize4.CreateCommitted(L"Depth Down-Sized 4", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R32_FLOAT);
    g_DepthTiled1.CreateCommittedArray(L"Depth De-Interleaved 1", bufferWidth[3], bufferHeight[3], 16, DXGI_FORMAT_R16_FLOAT);
    g_DepthTiled2.CreateCommittedArray(L"Depth De-Interleaved 2", bufferWidth[4], bufferHeight[4], 16, DXGI_FORMAT_R16_FLOAT);
    g_DepthTiled3.CreateCommittedArray(L"Depth De-Interleaved 3", bufferWidth[5], bufferHeight[5], 16, DXGI_FORMAT_R16_FLOAT);
    g_DepthTiled4.CreateCommittedArray(L"Depth De-Interleaved 4", bufferWidth[6], bufferHeight[6], 16, DXGI_FORMAT_R16_FLOAT);
    g_AOMerged1.CreateCommitted(L"AO Re-Interleaved 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_AOMerged2.CreateCommitted(L"AO Re-Interleaved 2", bufferWidth[2], bufferHeight[2], 1, DXGI_FORMAT_R8_UNORM);
    g_AOMerged3.CreateCommitted(L"AO Re-Interleaved 3", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R8_UNORM);
    g_AOMerged4.CreateCommitted(L"AO Re-Interleaved 4", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R8_UNORM);
    g_AOSmooth1.CreateCommitted(L"AO Smoothed 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_AOSmooth2.CreateCommitted(L"AO Smoothed 2", bufferWidth[2], bufferHeight[2], 1, DXGI_FORMAT_R8_UNORM);
    g_AOSmooth3.CreateCommitted(L"AO Smoothed 3", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R8_UNORM);
    g_AOHighQuality1.CreateCommitted(L"AO High Quality 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_AOHighQuality2.CreateCommitted(L"AO High Quality 2", bufferWidth[2], bufferHeight[2], 1, DXGI_FORMAT_R8_UNORM);
    g_AOHighQuality3.CreateCommitted(L"AO High Quality 3", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R8_UNORM);
    g_AOHighQuality4.CreateCommitted(L"AO High Quality 4", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R8_UNORM);

    g_ShadowBuffer.Create(L"Shadow Map", 2048, 2048);

    g_SceneDebugBuffer.CreateCommitted(L"Debug Buffer", bufferWidth[0], bufferHeight[0], 1, DefaultColorFormat, true);
    
    g_DoFTileClass[0].CreateCommitted(L"DoF Tile Classification Buffer 0", bufferWidth[4], bufferHeight[4], 1, DefaultColorFormat);
    g_DoFTileClass[1].CreateCommitted(L"DoF Tile Classification Buffer 1", bufferWidth[4], bufferHeight[4], 1, DefaultColorFormat);
    g_DoFTileClass[2].CreateCommitted(L"DoF Tile Classification Buffer 2", bufferWidth[4], bufferHeight[4], 1, DefaultColorFormat);

    g_DoFPresortBuffer.CreateCommitted(L"DoF Presort Buffer", bufferWidth[1], bufferHeight[1], 1, DefaultColorFormat);
    g_DoFPrefilter.CreateCommitted(L"DoF PreFilter Buffer", bufferWidth[1], bufferHeight[1], 1, DefaultColorFormat);
    g_DoFBlurColor[0].CreateCommitted(L"DoF Blur Color 0", bufferWidth[1], bufferHeight[1], 1, DefaultColorFormat);
    g_DoFBlurColor[1].CreateCommitted(L"DoF Blur Color 1", bufferWidth[1], bufferHeight[1], 1, DefaultColorFormat);
    g_DoFBlurColor[2].CreateCommitted(L"DoF Blur Color 2", bufferWidth[1], bufferHeight[1], 1, DefaultColorFormat);
    g_DoFBlurAlpha[0].CreateCommitted(L"DoF FG Alpha 0", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_DoFBlurAlpha[1].CreateCommitted(L"DoF FG Alpha 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_DoFBlurAlpha[2].CreateCommitted(L"DoF FG Alpha 2", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_DoFWorkQueue.CreateUAVCommitted(L"DoF Work Queue", bufferWidth[4] * bufferHeight[4], 4);
    g_DoFFastQueue.CreateUAVCommitted(L"DoF Fast Queue", bufferWidth[4] * bufferHeight[4], 4);
    g_DoFFixupQueue.CreateUAVCommitted(L"DoF Fixup Queue", bufferWidth[4] * bufferHeight[4], 4);

    g_TemporalColor[0].CreateCommitted(L"Temporal Color 0", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
    g_TemporalColor[1].CreateCommitted(L"Temporal Color 1", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
    g_TemporalColor[2].CreateCommitted(L"Temporal Color 2", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16G16B16A16_FLOAT);

    g_MotionPrepBuffer.CreateCommitted(L"Motion Blur Prep", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R16G16B16A16_FLOAT);

    // This is useful for storing per-pixel weights such as motion strength or pixel luminance
    g_LumaBuffer.CreateCommitted(L"Luminance", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R8_UNORM);
    g_Histogram.CreateUAVCommitted(L"Histogram", 256, 4);

    uint32_t kBloomWidth = (2560 < bufferWidth[0]) ? HashInternal::AlignUp(bufferWidth[0] / 4, 128) : 640;
    uint32_t kBloomHeight = (1440 < bufferHeight[0]) ? HashInternal::AlignUp(bufferHeight[0] / 4, 128) : 384;

    g_LumaLR.CreateCommitted(L"Luma Buffer", kBloomWidth, kBloomHeight, 1, DXGI_FORMAT_R8_UINT);
    g_aBloomUAV1[0].CreateCommitted(L"Bloom Buffer 1a", kBloomWidth, kBloomHeight, 1, DefaultColorFormat);
    g_aBloomUAV1[1].CreateCommitted(L"Bloom Buffer 1b", kBloomWidth, kBloomHeight, 1, DefaultColorFormat);
    g_aBloomUAV1[2].CreateCommitted(L"Bloom Buffer 1c", kBloomWidth, kBloomHeight, 1, DefaultColorFormat);
    g_aBloomUAV2[0].CreateCommitted(L"Bloom Buffer 2a", kBloomWidth / 2, kBloomHeight / 2, 1, DefaultColorFormat);
    g_aBloomUAV2[1].CreateCommitted(L"Bloom Buffer 2b", kBloomWidth / 2, kBloomHeight / 2, 1, DefaultColorFormat);
    g_aBloomUAV2[2].CreateCommitted(L"Bloom Buffer 2c", kBloomWidth / 2, kBloomHeight / 2, 1, DefaultColorFormat);
    g_aBloomUAV3[0].CreateCommitted(L"Bloom Buffer 3a", kBloomWidth / 4, kBloomHeight / 4, 1, DefaultColorFormat);
    g_aBloomUAV3[1].CreateCommitted(L"Bloom Buffer 3b", kBloomWidth / 4, kBloomHeight / 4, 1, DefaultColorFormat);
    g_aBloomUAV3[2].CreateCommitted(L"Bloom Buffer 3c", kBloomWidth / 4, kBloomHeight / 4, 1, DefaultColorFormat);
    g_aBloomUAV4[0].CreateCommitted(L"Bloom Buffer 4a", kBloomWidth / 8, kBloomHeight / 8, 1, DefaultColorFormat);
    g_aBloomUAV4[1].CreateCommitted(L"Bloom Buffer 4b", kBloomWidth / 8, kBloomHeight / 8, 1, DefaultColorFormat);
    g_aBloomUAV4[2].CreateCommitted(L"Bloom Buffer 4c", kBloomWidth / 8, kBloomHeight / 8, 1, DefaultColorFormat);
    g_aBloomUAV5[0].CreateCommitted(L"Bloom Buffer 5a", kBloomWidth / 16, kBloomHeight / 16, 1, DefaultColorFormat);
    g_aBloomUAV5[1].CreateCommitted(L"Bloom Buffer 5b", kBloomWidth / 16, kBloomHeight / 16, 1, DefaultColorFormat);
    g_aBloomUAV5[2].CreateCommitted(L"Bloom Buffer 5c", kBloomWidth / 16, kBloomHeight / 16, 1, DefaultColorFormat);

    const uint32_t kFXAAWorkSize = bufferWidth[0] * bufferHeight[0] / 4 + 128;
    g_FXAAWorkQueue.CreateUAVCommitted(L"FXAA Work Queue", kFXAAWorkSize, sizeof(uint32_t));
    g_FXAAColorQueue.CreateUAVCommitted(L"FXAA Color Queue", kFXAAWorkSize, sizeof(uint32_t));

    g_GenMipsBuffer.CreateCommitted(L"GenMips", bufferWidth[0], bufferHeight[0], 0, DXGI_FORMAT_R11G11B10_FLOAT);
    
    g_OverlayBuffer.CreateCommitted(L"g_OverlayBuffer", window::g_TargetWindowWidth, window::g_TargetWindowHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM, true);
    g_HorizontalBuffer.CreateCommitted(L"g_HorizontalBuffer", window::g_TargetWindowWidth, bufferHeight[0], 1, DefaultColorFormat);
}

void bufferManager::DestroyRenderingBuffers()
{
    g_SceneDepthBuffer.Destroy();
    g_SceneColorBuffer.Destroy();
    g_VelocityBuffer.Destroy();
    g_OverlayBuffer.Destroy();
    g_HorizontalBuffer.Destroy();
    g_PostEffectsBuffer.Destroy();

    g_ShadowBuffer.Destroy();
    g_SceneDebugBuffer.Destroy();
    g_OutlineBuffer.Destroy();
    g_OutlineHelpBuffer.Destroy();

    g_SSAOFullScreen.Destroy();
    g_LinearDepth[0].Destroy();
    g_LinearDepth[1].Destroy();
    g_LinearDepth[2].Destroy();
    g_MinMaxDepth8.Destroy();
    g_MinMaxDepth16.Destroy();
    g_MinMaxDepth32.Destroy();
    g_DepthDownsize1.Destroy();
    g_DepthDownsize2.Destroy();
    g_DepthDownsize3.Destroy();
    g_DepthDownsize4.Destroy();
    g_DepthTiled1.Destroy();
    g_DepthTiled2.Destroy();
    g_DepthTiled3.Destroy();
    g_DepthTiled4.Destroy();
    g_AOMerged1.Destroy();
    g_AOMerged2.Destroy();
    g_AOMerged3.Destroy();
    g_AOMerged4.Destroy();
    g_AOSmooth1.Destroy();
    g_AOSmooth2.Destroy();
    g_AOSmooth3.Destroy();
    g_AOHighQuality1.Destroy();
    g_AOHighQuality2.Destroy();
    g_AOHighQuality3.Destroy();
    g_AOHighQuality4.Destroy();

    g_DoFTileClass[0].Destroy();
    g_DoFTileClass[1].Destroy();
    g_DoFTileClass[2].Destroy();
    g_DoFPresortBuffer.Destroy();
    g_DoFPrefilter.Destroy();
    g_DoFBlurColor[0].Destroy();
    g_DoFBlurColor[1].Destroy();
    g_DoFBlurColor[2].Destroy();
    g_DoFBlurAlpha[0].Destroy();
    g_DoFBlurAlpha[1].Destroy();
    g_DoFBlurAlpha[2].Destroy();
    g_DoFWorkQueue.Destroy();
    g_DoFFastQueue.Destroy();
    g_DoFFixupQueue.Destroy();

    g_MotionPrepBuffer.Destroy();
    g_LumaBuffer.Destroy();
    g_TemporalColor[0].Destroy();
    g_TemporalColor[1].Destroy();
    g_TemporalColor[2].Destroy();
    g_aBloomUAV1[0].Destroy();
    g_aBloomUAV1[1].Destroy();
    g_aBloomUAV1[2].Destroy();
    g_aBloomUAV2[0].Destroy();
    g_aBloomUAV2[1].Destroy();
    g_aBloomUAV2[2].Destroy();
    g_aBloomUAV3[0].Destroy();
    g_aBloomUAV3[1].Destroy();
    g_aBloomUAV3[2].Destroy();
    g_aBloomUAV4[0].Destroy();
    g_aBloomUAV4[1].Destroy();
    g_aBloomUAV4[2].Destroy();
    g_aBloomUAV5[0].Destroy();
    g_aBloomUAV5[1].Destroy();
    g_aBloomUAV5[2].Destroy();
    g_LumaLR.Destroy();
    g_Histogram.Destroy();
    g_FXAAWorkCounters.Destroy();
    g_FXAAWorkQueue.Destroy();
    g_FXAAColorQueue.Destroy();

    g_GenMipsBuffer.Destroy();
}

void bufferManager::InitializeLightBuffers()
{
}

void bufferManager::DestroyLightBuffers()
{
    g_LightBuffer.Destroy();      // lightBuffer         : register(t66);
    g_LightShadowArray.Destroy(); // lightShadowArrayTex : register(t67);
    g_CumulativeShadowBuffer.Destroy();

    g_LightGrid.Destroy();        // lightGrid           : register(t68);
    g_LightGridBitMask.Destroy(); // lightGridBitMask    : register(t69);

    bufferManager::g_Lights.clear();
    bufferManager::g_LightShadowMatrixes.clear();
}