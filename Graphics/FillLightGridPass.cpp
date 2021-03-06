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

FillLightGridPass* FillLightGridPass::s_pFillLightGridPass = nullptr;

FillLightGridPass::FillLightGridPass(std::string name)
    : 
    ID3D12ScreenPass(name),
    m_kMinWorkGroupSize(8u),
    m_WorkGroupSize(16u)
{
    ASSERT(s_pFillLightGridPass == nullptr);

    s_pFillLightGridPass = this;

    m_FillLightRootSignature.Reset(3, 0);
    m_FillLightRootSignature[0].InitAsConstantBuffer(0);
    m_FillLightRootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    m_FillLightRootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    m_FillLightRootSignature.Finalize(L"FillLightRS");

    m_FillLightGridPSO_WORK_GROUP_8.SetRootSignature(m_FillLightRootSignature);
    m_FillLightGridPSO_WORK_GROUP_8.SetComputeShader(g_pFillLightGridCS_8, sizeof(g_pFillLightGridCS_8));
    m_FillLightGridPSO_WORK_GROUP_8.Finalize(L"FillLightGrid8_PSO");

    m_FillLightGridPSO_WORK_GROUP_16.SetRootSignature(m_FillLightRootSignature);
    m_FillLightGridPSO_WORK_GROUP_16.SetComputeShader(g_pFillLightGridCS_16, sizeof(g_pFillLightGridCS_16));
    m_FillLightGridPSO_WORK_GROUP_16.Finalize(L"FillLightGrid16_PSO");

    m_FillLightGridPSO_WORK_GROUP_24.SetRootSignature(m_FillLightRootSignature);
    m_FillLightGridPSO_WORK_GROUP_24.SetComputeShader(g_pFillLightGridCS_24, sizeof(g_pFillLightGridCS_24));
    m_FillLightGridPSO_WORK_GROUP_24.Finalize(L"FillLightGrid24_PSO");

    m_FillLightGridPSO_WORK_GROUP_32.SetRootSignature(m_FillLightRootSignature);
    m_FillLightGridPSO_WORK_GROUP_32.SetComputeShader(g_pFillLightGridCS_32, sizeof(g_pFillLightGridCS_32));
    m_FillLightGridPSO_WORK_GROUP_32.Finalize(L"FillLightGrid32_PSO");
}

// ��� RenderWindow �帧 Ȯ��
void FillLightGridPass::RenderWindow()
{
    ImGui::BeginChild("FillLightGridPass", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

    static uint32_t CurrentIndex = (m_WorkGroupSize / 8) - 1;
    static const char* WorkingGroups[4] = { "8", "16", "24", "32" };
    static const size_t WorkingGroupSize = _countof(WorkingGroups);
    if (ImGui::BeginCombo("Working Group Size", WorkingGroups[CurrentIndex]))
    {
        // must keep in sync with HLSL ( 8, 16, 24, 32 )
        for (uint32_t i = 0; i < WorkingGroupSize; ++i)
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

void FillLightGridPass::ExecutePass()
{
#ifdef _DEBUG
    graphics::InitDebugStartTick();
    float DeltaTime1 = graphics::GetDebugFrameTime();
#endif

    if (!bufferManager::g_LightBuffer.GetResource())
    {
#ifdef _DEBUG
        float DeltaTime2 = graphics::GetDebugFrameTime();
        m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
        return;
    }

    custom::ComputeContext& computeContext = custom::ComputeContext::Begin(1);
    computeContext.SetResourceTransitionBarrierIndex(0u);

    computeContext.PIXBeginEvent(L"5_FillLightPass", 0u);
    computeContext.SetRootSignature(m_FillLightRootSignature, 0u);

    switch (m_WorkGroupSize)
    {
    case  8u: computeContext.SetPipelineState(m_FillLightGridPSO_WORK_GROUP_8,  0u); break;
    case 16u: computeContext.SetPipelineState(m_FillLightGridPSO_WORK_GROUP_16, 0u); break;
    case 24u: computeContext.SetPipelineState(m_FillLightGridPSO_WORK_GROUP_24, 0u); break;
    case 32u: computeContext.SetPipelineState(m_FillLightGridPSO_WORK_GROUP_32, 0u); break;
    default: ASSERT(false);
        break;
    }

    // g_LinearDepth is Written by SSAOPass.
    ColorBuffer& LinearDepth = bufferManager::g_LinearDepth[graphics::GetFrameCount() % device::g_DisplayBufferCount];

    computeContext.TransitionResources(bufferManager::g_LightBuffer,      D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES); // 
    computeContext.TransitionResource(LinearDepth,                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0U);
    computeContext.TransitionResources(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_BARRIER_DEPTH_STENCIL_SUBRESOURCE_BITFLAG);
    computeContext.TransitionResource(bufferManager::g_LightGrid,         D3D12_RESOURCE_STATE_COMMON, 0U); //
    computeContext.TransitionResource(bufferManager::g_LightGridBitMask,  D3D12_RESOURCE_STATE_COMMON, 0U); //
    computeContext.SubmitResourceBarriers(0u);

    // computeContext.TransitionResource(bufferManager::g_LightBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); // 
    // computeContext.TransitionResource(LinearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    // computeContext.TransitionResource(bufferManager::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    // computeContext.TransitionResource(bufferManager::g_LightGrid, D3D12_RESOURCE_STATE_UNORDERED_ACCESS); // 
    // computeContext.TransitionResource(bufferManager::g_LightGridBitMask, D3D12_RESOURCE_STATE_UNORDERED_ACCESS); //
    // computeContext.SubmitResourceBarriers(0u);

    computeContext.SetDynamicDescriptor(1, 0, bufferManager::g_LightBuffer.GetSRV(), 0u);
    computeContext.SetDynamicDescriptor(1, 1, LinearDepth.GetSRV(), 0u);
    computeContext.SetDynamicDescriptor(2, 0, bufferManager::g_LightGrid.GetUAV(), 0u);
    computeContext.SetDynamicDescriptor(2, 1, bufferManager::g_LightGridBitMask.GetUAV(), 0u);

    // todo: assumes 1920x1080 resolution
    uint32_t tileCountX = Math::DivideByMultiple(bufferManager::g_SceneColorBuffer.GetWidth(),  m_WorkGroupSize);
    uint32_t tileCountY = Math::DivideByMultiple(bufferManager::g_SceneColorBuffer.GetHeight(), m_WorkGroupSize);

    Camera* pMainCamera = computeContext.GetpMainCamera();

    float RcpZMagic;
	{
		float FarClipDist = pMainCamera->GetFarZClip();
		float NearClipDist = pMainCamera->GetNearZClip();
		RcpZMagic = NearClipDist / (FarClipDist - NearClipDist);
	}

    CSConstants csConstants;
    // todo: assumes 1920x1080 resolution
    csConstants.ViewportWidth  = bufferManager::g_SceneColorBuffer.GetWidth();
    csConstants.ViewportHeight = bufferManager::g_SceneColorBuffer.GetHeight();
    csConstants.InvTileDim     = 1.0f / m_WorkGroupSize;
    csConstants.RcpZMagic      = RcpZMagic;
    csConstants.TileCount      = tileCountX;
    csConstants.ViewProjMatrix = pMainCamera->GetViewProjMatrix();

    computeContext.SetDynamicConstantBufferView(0U, sizeof(CSConstants), &csConstants, 0u);
    computeContext.Dispatch((size_t)tileCountX, (size_t)tileCountY, 1ul, 0u);
    computeContext.PIXEndEvent(0u); // End FillLightGridPass
    computeContext.Finish(false);

#ifdef _DEBUG
    float DeltaTime2 = graphics::GetDebugFrameTime();
    m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}

void FillLightGridPass::Reset() DEBUG_EXCEPT
{

}