#pragma once

// Keep in sync HLSL
struct VSConstants
{
    VSConstants()
    {
        memset(&modelToProjection, -1, sizeof(Math::Matrix4));
        memset(&ViewMatrix,        -1, sizeof(Math::Matrix4));
        memset(&modelToShadow,     -1, sizeof(Math::Matrix4));
        memset(&viewerPos,         -1, sizeof(DirectX::XMFLOAT3));
    }

    Math::Matrix4     modelToProjection; // m_ProjMatrix(Frustum) * DirectX::XMMatrixLookToLH => VP
    Math::Matrix4     ViewMatrix;
    Math::Matrix4     modelToShadow;
    DirectX::XMFLOAT3 viewerPos;     // m_Camera.m_CameraToWorld.GetTranslation()
};

__declspec(align(16)) struct PSConstants
{
    PSConstants()
    {
        memset(&sunDirection, -1, sizeof(Math::Vector3));
        memset(&sunLightColor, -1, sizeof(Math::Vector3));
        memset(&ambientColor, -1, sizeof(Math::Vector3));

        ShadowTexelSize[0] = 0.0f;
        InvTileDim[0] = 0.0f;
        TileCount[0] = -1;
        FirstLightIndex[0] = -1;
        FirstLightIndex[1] = -1;
    }

    Math::Vector3 sunDirection; // Declaration at MasterRenderGraph
    Math::Vector3 sunLightColor;
    Math::Vector3 ambientColor;
    float ShadowTexelSize[4]; // ShadowTexelSize[0] = Inv of ShadowBuffer.Width()

    // keep in sync with HLSL
    // Padding 2Bytes => Orgin InvTileDim[4] // 1.0f / LightPrePass::m_WorkGroupSize
    // Padding 2Bytes => Orgin TileCount[4] // 1.0f / LightPrePass::m_WorkGroupSize
    float InvTileDim[2];
    // TileCount[0] = HashInternal::DivideByMultiple(bufferManager::g_SceneColorBuffer.GetWidth(), Lighting::LightGridDim);
    // TileCount[1] = HashInternal::DivideByMultiple(bufferManager::g_SceneColorBuffer.GetHeight(), Lighting::LightGridDim);
    uint32_t TileCount[2];

    // Padding 2Bytes
    // FirstLightIndex[0] = LightPrePass::m_FirstConeLightIndex
    // FirstLightIndex[1] = LightPrePass::m_FirstConeShadowedLightIndex
    uint32_t FirstLightIndex[2];
    // uint32_t FrameIndexMod2;
};

struct CSConstants
{
    uint32_t ViewportWidth, ViewportHeight;
    float InvTileDim;
    float RcpZMagic;
    uint32_t TileCount;
    Math::Matrix4 ViewProjMatrix;
};


struct alignas(128) LightData
{
    DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
    float radiusSq = 100000.0f;
    DirectX::XMFLOAT3 color = { 0.31f, 0.27f, 0.25f };
    uint32_t type = 0; // 0 Sphere, 1 Cone, 2 Cone with Shadow Map

    DirectX::XMFLOAT3 coneDir = {-0.5f, -0.1f, -0.8f };
    float Padding0;
    DirectX::XMFLOAT2 coneAngles = { 50.0f, 0.91f };
    float Padding1[2];
    float shadowTextureMatrix[16];
};