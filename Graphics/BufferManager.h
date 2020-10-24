#pragma once
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "ShadowBuffer.h"
#include "UAVBuffer.h"
#include "ShaderConstantsTypeDefinitions.h"

namespace bufferManager
{
    extern DepthBuffer g_SceneDepthBuffer;  // D32_FLOAT_S8_UINT
    extern ColorBuffer g_SceneColorBuffer;  // R11G11B10_FLOAT
    extern ColorBuffer g_PostEffectsBuffer; // R32_UINT (to support Read-Modify-Write with a UAV)
    extern ColorBuffer g_OverlayBuffer;     // R8G8B8A8_UNORM
    extern ColorBuffer g_HorizontalBuffer;  // For separable (bicubic) upsampling

    extern ColorBuffer  g_VelocityBuffer;   // R10G10B10  (3D velocity)
    extern ShadowBuffer g_ShadowBuffer;

    extern ColorBuffer g_StencilBuffer;

    extern ColorBuffer g_SSAOFullScreen;    // R8_UNORM
    extern ColorBuffer g_LinearDepth[2];    // Normalized planar distance (0 at eye, 1 at far plane) 
                                            // computed from the SceneDepthBuffer

    extern ColorBuffer g_MinMaxDepth8;      // Min and max depth values of 8x8 tiles
    extern ColorBuffer g_MinMaxDepth16;     // Min and max depth values of 16x16 tiles
    extern ColorBuffer g_MinMaxDepth32;     // Min and max depth values of 16x16 tiles
    extern ColorBuffer g_DepthDownsize1;
    extern ColorBuffer g_DepthDownsize2;
    extern ColorBuffer g_DepthDownsize3;
    extern ColorBuffer g_DepthDownsize4;
    extern ColorBuffer g_DepthTiled1;
    extern ColorBuffer g_DepthTiled2;
    extern ColorBuffer g_DepthTiled3;
    extern ColorBuffer g_DepthTiled4;
    extern ColorBuffer g_AOMerged1;
    extern ColorBuffer g_AOMerged2;
    extern ColorBuffer g_AOMerged3;
    extern ColorBuffer g_AOMerged4;
    extern ColorBuffer g_AOSmooth1;
    extern ColorBuffer g_AOSmooth2;
    extern ColorBuffer g_AOSmooth3;
    extern ColorBuffer g_AOHighQuality1;
    extern ColorBuffer g_AOHighQuality2;
    extern ColorBuffer g_AOHighQuality3;
    extern ColorBuffer g_AOHighQuality4;

    extern ColorBuffer g_DoFTileClass[2];
    extern ColorBuffer g_DoFPresortBuffer;
    extern ColorBuffer g_DoFPrefilter;
    extern ColorBuffer g_DoFBlurColor[2];
    extern ColorBuffer g_DoFBlurAlpha[2];

    extern custom::StructuredBuffer g_DoFWorkQueue;
    extern custom::StructuredBuffer g_DoFFastQueue;
    extern custom::StructuredBuffer g_DoFFixupQueue;

    extern ColorBuffer g_MotionPrepBuffer;        // R10G10B10A2
    extern ColorBuffer g_LumaBuffer;
    extern ColorBuffer g_TemporalColor[2];

    extern ColorBuffer g_aBloomUAV1[2];        // 640x384 (1/3)
    extern ColorBuffer g_aBloomUAV2[2];        // 320x192 (1/6)  
    extern ColorBuffer g_aBloomUAV3[2];        // 160x96  (1/12)
    extern ColorBuffer g_aBloomUAV4[2];        // 80x48   (1/24)
    extern ColorBuffer g_aBloomUAV5[2];        // 40x24   (1/48)
    extern ColorBuffer g_LumaLR;

    extern custom::ByteAddressBuffer g_Histogram;
    extern custom::ByteAddressBuffer g_FXAAWorkCounters;
    extern custom::ByteAddressBuffer g_FXAAWorkQueue;
    extern custom::TypedBuffer g_FXAAColorQueue;

    extern ColorBuffer g_GenMipsBuffer;

    // LightPrePass
    extern std::vector<LightData>     g_Lights;               
    extern std::vector<Math::Matrix4> g_LightShadowMatrixes;

    extern custom::StructuredBuffer   g_LightBuffer;      // lightBuffer         : register(t66);
    extern ColorBuffer                g_LightShadowArray; // lightShadowArrayTex : register(t67);
    extern ShadowBuffer               g_CumulativeShadowBuffer;

    extern custom::ByteAddressBuffer  g_LightGrid;        // lightGrid           : register(t68);
    extern custom::ByteAddressBuffer  g_LightGridBitMask; // lightGridBitMask    : register(t69);

    void InitializeAllBuffers(uint32_t Width, uint32_t Height);
    void DestroyRenderingBuffers();
}