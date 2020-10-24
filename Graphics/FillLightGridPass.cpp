#include "stdafx.h"
#include "FillLightGridPass.h"
#include "CommandContext.h"
#include "ComputeContext.h"
#include "BufferManager.h"
#include "Camera.h"
#include "ShaderConstantsTypeDefinitions.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/FillLightGridCS_8.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/FillLightGridCS_16.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/FillLightGridCS_24.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/FillLightGridCS_32.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/FillLightGridCS_8.h"
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/FillLightGridCS_16.h"
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/FillLightGridCS_24.h"
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/FillLightGridCS_32.h"
#endif

FillLightGridPass::FillLightGridPass(std::string pName)
    : Pass(pName)
{
    m_FillLightRootSignature.Reset(3, 0);
    m_FillLightRootSignature[0].InitAsConstantBuffer(0);
    m_FillLightRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    m_FillLightRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    m_FillLightRootSignature.Finalize(L"FillLightRS");

    m_FillLightGridPSO_WORK_GROUP_8.SetRootSignature(m_FillLightRootSignature);
    m_FillLightGridPSO_WORK_GROUP_8.SetComputeShader(g_pFillLightGridCS_8, sizeof(g_pFillLightGridCS_8));
    m_FillLightGridPSO_WORK_GROUP_8.Finalize();

    m_FillLightGridPSO_WORK_GROUP_16.SetRootSignature(m_FillLightRootSignature);
    m_FillLightGridPSO_WORK_GROUP_16.SetComputeShader(g_pFillLightGridCS_16, sizeof(g_pFillLightGridCS_16));
    m_FillLightGridPSO_WORK_GROUP_16.Finalize();

    m_FillLightGridPSO_WORK_GROUP_24.SetRootSignature(m_FillLightRootSignature);
    m_FillLightGridPSO_WORK_GROUP_24.SetComputeShader(g_pFillLightGridCS_24, sizeof(g_pFillLightGridCS_24));
    m_FillLightGridPSO_WORK_GROUP_24.Finalize();

    m_FillLightGridPSO_WORK_GROUP_32.SetRootSignature(m_FillLightRootSignature);
    m_FillLightGridPSO_WORK_GROUP_32.SetComputeShader(g_pFillLightGridCS_32, sizeof(g_pFillLightGridCS_32));
    m_FillLightGridPSO_WORK_GROUP_32.Finalize();
}

// 모든 RenderWindow 흐름 확인
void FillLightGridPass::RenderWindow()
{
    ImGui::BeginChild("FillLightGridPass", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

    static uint32_t CurrentIndex = (m_WorkGroupSize / 8) - 1;
    static const char* WorkingGroups[4] = { "8", "16", "24", "32" };
    static const size_t WorkingGroupSize = _countof(WorkingGroups);
    if (ImGui::BeginCombo("Working Group Size", WorkingGroups[CurrentIndex]))
    {
        // must keep in sync with HLSL ( 8, 16, 24, 32 )
        for (size_t i = 0; i < WorkingGroupSize; ++i)
        {
            const bool bSelected = (CurrentIndex == i);
            if (ImGui::Selectable(WorkingGroups[i], bSelected))
            {
                m_WorkGroupSize = (i + 1) * 8;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::EndChild();
}

void FillLightGridPass::Execute(custom::CommandContext& BaseContext)
{
#ifdef _DEBUG
    graphics::InitDebugStartTick();
    float DeltaTime1 = graphics::GetDebugFrameTime();
#endif
    custom::ComputeContext& computeContext = BaseContext.GetComputeContext();

    // Move to MasterRenderGraph::Update();
    // computeContext.SetTileDimension(bufferManager::g_SceneColorBuffer.GetWidth(), bufferManager::g_SceneColorBuffer.GetHeight(), m_WorkGroupSize);

    computeContext.PIXBeginEvent(L"5_FillLightPass");

    if (!bufferManager::g_LightBuffer.GetResource())
    {
        return;
    }

    computeContext.SetRootSignature(m_FillLightRootSignature);

    switch ((int)m_WorkGroupSize)
    {
    case  8: computeContext.SetPipelineState(m_FillLightGridPSO_WORK_GROUP_8); break;
    case 16: computeContext.SetPipelineState(m_FillLightGridPSO_WORK_GROUP_16); break;
    case 24: computeContext.SetPipelineState(m_FillLightGridPSO_WORK_GROUP_24); break;
    case 32: computeContext.SetPipelineState(m_FillLightGridPSO_WORK_GROUP_32); break;
    default: ASSERT(false);
        break;
    }

    ColorBuffer& LinearDepth = bufferManager::g_LinearDepth[graphics::GetFrameCount() % 2];

    computeContext.TransitionResource(bufferManager::g_LightBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    computeContext.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    computeContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    computeContext.TransitionResource(bufferManager::g_LightGrid, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    computeContext.TransitionResource(bufferManager::g_LightGridBitMask, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    computeContext.SetDynamicDescriptor(1, 0, bufferManager::g_LightBuffer.GetSRV());
    computeContext.SetDynamicDescriptor(1, 1, LinearDepth.GetSRV());
    computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_LightGrid.GetUAV());
    computeContext.SetDynamicDescriptor(2, 1, bufferManager::g_LightGridBitMask.GetUAV());

    // todo: assumes 1920x1080 resolution
    uint32_t tileCountX = Math::DivideByMultiple(bufferManager::g_SceneColorBuffer.GetWidth(), m_WorkGroupSize);
    uint32_t tileCountY = Math::DivideByMultiple(bufferManager::g_SceneColorBuffer.GetHeight(), m_WorkGroupSize);

    Camera* pMainCamera = BaseContext.GetpMainCamera();

    float FarClipDist = pMainCamera->GetFarZClip();
    float NearClipDist = pMainCamera->GetNearZClip();
    const float RcpZMagic = NearClipDist / (FarClipDist - NearClipDist);

    CSConstants csConstants;
    // todo: assumes 1920x1080 resolution
    csConstants.ViewportWidth = bufferManager::g_SceneColorBuffer.GetWidth();
    csConstants.ViewportHeight = bufferManager::g_SceneColorBuffer.GetHeight();
    csConstants.InvTileDim = 1.0f / m_WorkGroupSize;
    csConstants.RcpZMagic = RcpZMagic;
    csConstants.TileCount = tileCountX;
    csConstants.ViewProjMatrix = pMainCamera->GetViewProjMatrix();

    computeContext.SetDynamicConstantBufferView(0, sizeof(CSConstants), &csConstants);

    computeContext.Dispatch(tileCountX, tileCountY, 1);

    computeContext.TransitionResource(bufferManager::g_LightGrid, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    computeContext.TransitionResource(bufferManager::g_LightGridBitMask, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    computeContext.PIXEndEvent();
#ifdef _DEBUG
    float DeltaTime2 = graphics::GetDebugFrameTime();
    m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}

void FillLightGridPass::Reset() DEBUG_EXCEPT
{

}