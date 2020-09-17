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
     ColorBuffer g_PostEffectsBuffer;    // R32_UINT (to support Read-Modify-Write with a UAV)
     ColorBuffer g_OverlayBuffer;        // R8G8B8A8_UNORM
     ColorBuffer g_HorizontalBuffer;    // For separable (bicubic) upsampling

     ColorBuffer  g_VelocityBuffer;    // R10G10B10  (3D velocity)
     ShadowBuffer g_ShadowBuffer;

     ColorBuffer g_StencilBuffer;

     ColorBuffer g_SSAOFullScreen(custom::Color(1.0f, 1.0f, 1.0f));    // R8_UNORM
     ColorBuffer g_LinearDepth[2];    // Normalized planar distance (0 at eye, 1 at far plane) computed from the SceneDepthBuffer
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

     ColorBuffer g_DoFTileClass[2];
     ColorBuffer g_DoFPresortBuffer;
     ColorBuffer g_DoFPrefilter;
     ColorBuffer g_DoFBlurColor[2];
     ColorBuffer g_DoFBlurAlpha[2];
     custom::StructuredBuffer g_DoFWorkQueue;
     custom::StructuredBuffer g_DoFFastQueue;
     custom::StructuredBuffer g_DoFFixupQueue;

     ColorBuffer g_MotionPrepBuffer;        // R10G10B10A2
     ColorBuffer g_LumaBuffer;
     ColorBuffer g_TemporalColor[2];

     ColorBuffer g_aBloomUAV1[2];        // 640x384 (1/3)
     ColorBuffer g_aBloomUAV2[2];        // 320x192 (1/6)  
     ColorBuffer g_aBloomUAV3[2];        // 160x96  (1/12)
     ColorBuffer g_aBloomUAV4[2];        // 80x48   (1/24)
     ColorBuffer g_aBloomUAV5[2];        // 40x24   (1/48)
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

     constexpr static DXGI_FORMAT DefaultColorFormat{ DXGI_FORMAT_R11G11B10_FLOAT };
}

void bufferManager::InitializeAllBuffers(uint32_t Width, uint32_t Height)
{
    uint32_t startingPoint[14];

    uint32_t* bufferWidth = startingPoint;
    uint32_t* bufferHeight = (uint32_t*)startingPoint + 7;

    // bufferWidth = { Width, Width / 2, Width / 4, Width / 8, Width / 16, Width / 32, Width / 64 };
    // bufferHeight = { Height, Height / 2, Height / 4, Height / 8, Height / 16, Height / 32, Height / 64 };
    bufferWidth[0] = Width;
    bufferHeight[0] = Height;

    for (size_t i{ 1 }; i < 7; ++i)
    {
        bufferWidth[i] = bufferWidth[i - 1] >> 1;
    }
    
    for (size_t i{ 1 }; i < 7; ++i)
    {
        bufferHeight[i] = bufferHeight[i - 1] >> 1;
    }

    custom::GraphicsContext& graphicsContext = custom::GraphicsContext::Begin(L"Buffer Manager Initialize");

    g_SceneColorBuffer.Create(L"Main Color Buffer", bufferWidth[0], bufferHeight[0], 1, DefaultColorFormat);
    g_VelocityBuffer.Create(L"Motion Vectors", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R32_UINT);
    g_PostEffectsBuffer.Create(L"Post Effects Buffer", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R32_UINT);

    g_LinearDepth[0].Create(L"Linear Depth 0", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16_UNORM);
    g_LinearDepth[1].Create(L"Linear Depth 1", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16_UNORM);
    g_MinMaxDepth8.Create(L"MinMaxDepth 8x8", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R32_UINT);
    g_MinMaxDepth16.Create(L"MinMaxDepth 16x16", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R32_UINT);
    g_MinMaxDepth32.Create(L"MinMaxDepth 32x32", bufferWidth[5], bufferHeight[5], 1, DXGI_FORMAT_R32_UINT);

    g_SceneDepthBuffer.Create(L"Scene Depth Buffer", bufferWidth[0], bufferHeight[0], DXGI_FORMAT_D32_FLOAT);

    g_SSAOFullScreen.Create(L"SSAO Full Resolution", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R8_UNORM);

    g_DepthDownsize1.Create(L"Depth Down-Sized 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R32_FLOAT);
    g_DepthDownsize2.Create(L"Depth Down-Sized 2", bufferWidth[2], bufferHeight[2], 1, DXGI_FORMAT_R32_FLOAT);
    g_DepthDownsize3.Create(L"Depth Down-Sized 3", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R32_FLOAT);
    g_DepthDownsize4.Create(L"Depth Down-Sized 4", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R32_FLOAT);
    g_DepthTiled1.CreateArray(L"Depth De-Interleaved 1", bufferWidth[3], bufferHeight[3], 16, DXGI_FORMAT_R16_FLOAT);
    g_DepthTiled2.CreateArray(L"Depth De-Interleaved 2", bufferWidth[4], bufferHeight[4], 16, DXGI_FORMAT_R16_FLOAT);
    g_DepthTiled3.CreateArray(L"Depth De-Interleaved 3", bufferWidth[5], bufferHeight[5], 16, DXGI_FORMAT_R16_FLOAT);
    g_DepthTiled4.CreateArray(L"Depth De-Interleaved 4", bufferWidth[6], bufferHeight[6], 16, DXGI_FORMAT_R16_FLOAT);
    g_AOMerged1.Create(L"AO Re-Interleaved 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_AOMerged2.Create(L"AO Re-Interleaved 2", bufferWidth[2], bufferHeight[2], 1, DXGI_FORMAT_R8_UNORM);
    g_AOMerged3.Create(L"AO Re-Interleaved 3", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R8_UNORM);
    g_AOMerged4.Create(L"AO Re-Interleaved 4", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R8_UNORM);
    g_AOSmooth1.Create(L"AO Smoothed 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_AOSmooth2.Create(L"AO Smoothed 2", bufferWidth[2], bufferHeight[2], 1, DXGI_FORMAT_R8_UNORM);
    g_AOSmooth3.Create(L"AO Smoothed 3", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R8_UNORM);
    g_AOHighQuality1.Create(L"AO High Quality 1", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_AOHighQuality2.Create(L"AO High Quality 2", bufferWidth[2], bufferHeight[2], 1, DXGI_FORMAT_R8_UNORM);
    g_AOHighQuality3.Create(L"AO High Quality 3", bufferWidth[3], bufferHeight[3], 1, DXGI_FORMAT_R8_UNORM);
    g_AOHighQuality4.Create(L"AO High Quality 4", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R8_UNORM);

    g_ShadowBuffer.Create(L"Shadow Map", 2048, 2048);

    g_StencilBuffer.Create(L"Stencil Buffer", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_B8G8R8A8_UNORM);

    g_DoFTileClass[0].Create(L"DoF Tile Classification Buffer 0", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R11G11B10_FLOAT);
    g_DoFTileClass[1].Create(L"DoF Tile Classification Buffer 1", bufferWidth[4], bufferHeight[4], 1, DXGI_FORMAT_R11G11B10_FLOAT);

    g_DoFPresortBuffer.Create(L"DoF Presort Buffer", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R11G11B10_FLOAT);
    g_DoFPrefilter.Create(L"DoF PreFilter Buffer", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R11G11B10_FLOAT);
    g_DoFBlurColor[0].Create(L"DoF Blur Color", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R11G11B10_FLOAT);
    g_DoFBlurColor[1].Create(L"DoF Blur Color", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R11G11B10_FLOAT);
    g_DoFBlurAlpha[0].Create(L"DoF FG Alpha", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_DoFBlurAlpha[1].Create(L"DoF FG Alpha", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R8_UNORM);
    g_DoFWorkQueue.Create(L"DoF Work Queue", bufferWidth[4] * bufferHeight[4], 4);
    g_DoFFastQueue.Create(L"DoF Fast Queue", bufferWidth[4] * bufferHeight[4], 4);
    g_DoFFixupQueue.Create(L"DoF Fixup Queue", bufferWidth[4] * bufferHeight[4], 4);

    g_TemporalColor[0].Create(L"Temporal Color 0", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
    g_TemporalColor[1].Create(L"Temporal Color 1", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R16G16B16A16_FLOAT);

    g_MotionPrepBuffer.Create(L"Motion Blur Prep", bufferWidth[1], bufferHeight[1], 1, DXGI_FORMAT_R16G16B16A16_FLOAT);

    // This is useful for storing per-pixel weights such as motion strength or pixel luminance
    g_LumaBuffer.Create(L"Luminance", bufferWidth[0], bufferHeight[0], 1, DXGI_FORMAT_R8_UNORM);
    g_Histogram.Create(L"Histogram", 256, 4);

    uint32_t kBloomWidth = (2560 < bufferWidth[0]) ? HashInternal::AlignUp(bufferWidth[0] / 4, 128) : 640;
    uint32_t kBloomHeight = (1440 < bufferHeight[0]) ? HashInternal::AlignUp(bufferHeight[0] / 4, 128) : 384;

    g_LumaLR.Create(L"Luma Buffer", kBloomWidth, kBloomHeight, 1, DXGI_FORMAT_R8_UINT);
    g_aBloomUAV1[0].Create(L"Bloom Buffer 1a", kBloomWidth, kBloomHeight, 1, DefaultColorFormat);
    g_aBloomUAV1[1].Create(L"Bloom Buffer 1b", kBloomWidth, kBloomHeight, 1, DefaultColorFormat);
    g_aBloomUAV2[0].Create(L"Bloom Buffer 2a", kBloomWidth / 2, kBloomHeight / 2, 1, DefaultColorFormat);
    g_aBloomUAV2[1].Create(L"Bloom Buffer 2b", kBloomWidth / 2, kBloomHeight / 2, 1, DefaultColorFormat);
    g_aBloomUAV3[0].Create(L"Bloom Buffer 3a", kBloomWidth / 4, kBloomHeight / 4, 1, DefaultColorFormat);
    g_aBloomUAV3[1].Create(L"Bloom Buffer 3b", kBloomWidth / 4, kBloomHeight / 4, 1, DefaultColorFormat);
    g_aBloomUAV4[0].Create(L"Bloom Buffer 4a", kBloomWidth / 8, kBloomHeight / 8, 1, DefaultColorFormat);
    g_aBloomUAV4[1].Create(L"Bloom Buffer 4b", kBloomWidth / 8, kBloomHeight / 8, 1, DefaultColorFormat);
    g_aBloomUAV5[0].Create(L"Bloom Buffer 5a", kBloomWidth / 16, kBloomHeight / 16, 1, DefaultColorFormat);
    g_aBloomUAV5[1].Create(L"Bloom Buffer 5b", kBloomWidth / 16, kBloomHeight / 16, 1, DefaultColorFormat);

    const uint32_t kFXAAWorkSize = bufferWidth[0] * bufferHeight[0] / 4 + 128;
    g_FXAAWorkQueue.Create(L"FXAA Work Queue", kFXAAWorkSize, sizeof(uint32_t));
    g_FXAAColorQueue.Create(L"FXAA Color Queue", kFXAAWorkSize, sizeof(uint32_t));
    g_FXAAWorkCounters.Create(L"FXAA Work Counters", 2, sizeof(uint32_t));
    graphicsContext.ClearUAV(g_FXAAWorkCounters);

    g_GenMipsBuffer.Create(L"GenMips", bufferWidth[0], bufferHeight[0], 0, DXGI_FORMAT_R11G11B10_FLOAT);
    
    g_OverlayBuffer.Create(L"UI Overlay", window::g_windowWidth, window::g_windowHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
    g_HorizontalBuffer.Create(L"Bicubic Intermediate", window::g_windowWidth, bufferHeight[0], 1, DefaultColorFormat);

    ((custom::CommandContext)graphicsContext).Finish();
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

    g_SSAOFullScreen.Destroy();
    g_LinearDepth[0].Destroy();
    g_LinearDepth[1].Destroy();
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
    g_DoFPresortBuffer.Destroy();
    g_DoFPrefilter.Destroy();
    g_DoFBlurColor[0].Destroy();
    g_DoFBlurColor[1].Destroy();
    g_DoFBlurAlpha[0].Destroy();
    g_DoFBlurAlpha[1].Destroy();
    g_DoFWorkQueue.Destroy();
    g_DoFFastQueue.Destroy();
    g_DoFFixupQueue.Destroy();

    g_MotionPrepBuffer.Destroy();
    g_LumaBuffer.Destroy();
    g_TemporalColor[0].Destroy();
    g_TemporalColor[1].Destroy();
    g_aBloomUAV1[0].Destroy();
    g_aBloomUAV1[1].Destroy();
    g_aBloomUAV2[0].Destroy();
    g_aBloomUAV2[1].Destroy();
    g_aBloomUAV3[0].Destroy();
    g_aBloomUAV3[1].Destroy();
    g_aBloomUAV4[0].Destroy();
    g_aBloomUAV4[1].Destroy();
    g_aBloomUAV5[0].Destroy();
    g_aBloomUAV5[1].Destroy();
    g_LumaLR.Destroy();
    g_Histogram.Destroy();
    g_FXAAWorkCounters.Destroy();
    g_FXAAWorkQueue.Destroy();
    g_FXAAColorQueue.Destroy();

    g_GenMipsBuffer.Destroy();
}