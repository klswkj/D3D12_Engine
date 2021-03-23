#include "stdafx.h"
#include "LightPrePass.h"
#include "UAVBuffer.h"
#include "ColorBuffer.h"
#include "BufferManager.h"
#include "ShadowBuffer.h"
#include "PSO.h"
#include "RootSignature.h"
#include "CommandContext.h"
#include "ComputeContext.h"
#include "Camera.h"
#include "ShaderConstantsTypeDefinitions.h"

uint32_t LightPrePass::sm_MaxLight = 128u;
const char* LightPrePass::sm_LightLabel[3] = { "Sphere Light", "Cone Light", "Cone Light with ShadowMap" };

constexpr uint32_t LightPrePass::sm_kShadowBufferSize = 512u;
constexpr uint32_t LightPrePass::sm_kMinWorkGroupSize = 8u;

// sm_kLightGridCells = 0x00007e90
constexpr uint32_t LightPrePass::sm_kLightGridCells = 0x7e90u;
constexpr uint32_t LightPrePass::sm_lightGridBitMaskSizeBytes = 0x7e900u;

LightPrePass::LightPrePass(std::string pName)
    : D3D12Pass(pName)
{
    bufferManager::g_Lights.reserve((size_t)(sm_MaxLight * 2u + 1u));
    bufferManager::g_LightShadowMatrixes.reserve((size_t)(sm_MaxLight * 2u + 1u));

    CreateSphereLight();
    CreateConeLight();
    CreateConeShadowedLight();

    bufferManager::g_LightGridBitMask.CreateUAVCommitted(L"g_LightGridBitMask", sm_lightGridBitMaskSizeBytes, 1, nullptr);
    bufferManager::g_CumulativeShadowBuffer.Create(L"g_CumulativeShadowBuffer", sm_kShadowBufferSize, sm_kShadowBufferSize);
}

void LightPrePass::ExecutePass()
{
#ifdef _DEBUG
    graphics::InitDebugStartTick();
    float DeltaTime1 = graphics::GetDebugFrameTime();
#endif

    // TODO : If has a new light
    sortContainer();

#ifdef _DEBUG
    float DeltaTime2 = graphics::GetDebugFrameTime();
    m_DeltaTime = DeltaTime2 - DeltaTime1;
#endif
}

//////////////////////////////////////////////////////////
// LightPrePass                                         //
// Create Sphere Light
// Create Cone   Light

void LightPrePass::RenderWindow() DEBUG_EXCEPT
{
    /*
    ImGui::BeginChild("LightPrePass"); // 여기 Begin말고, ImGui::BeginChild해야, 

    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    ASSERT(m_Lights.size() == m_LightShadowMatrixes.size());

    const uint32_t bHasLight = (m_Lights.size() && m_LightShadowMatrixes.size());
    
    ImGui::Columns(2 + bHasLight, nullptr, true);
    {
        ImGui::BeginChild("", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

        if (m_Lights.size() < sm_MaxLight)
        {
            if (ImGui::Button("Create Sphere Light"))
            {
                CreateSphereLight();
            }
            if (ImGui::Button("Create Cone   Light"))
            {
                CreateConeLight();
            }
        }

        ImGui::EndChild();

    }

    static size_t SelectedLightIndex = -1;
    {
        ImGui::NextColumn();
        ImGui::BeginChild("", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

        const int Imgui_Node_Flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_FramePadding |
            ImGuiTreeNodeFlags_Leaf;

        for (size_t i= 0; i < m_Lights.size(); ++i)
        {
            if (ImGui::TreeNodeEx("Light", Imgui_Node_Flags, sm_LightLabel[m_Lights[i].type]))
            {
                SelectedLightIndex = i;
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
    }

    if (bHasLight && SelectedLightIndex != -1)
    {
        ImGui::NextColumn();
        ImGui::BeginChild("", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

        bool bDirty = false;
        const auto SetDirty = [](bool given, bool dirtybit) {dirtybit = dirtybit || given; };

        LightData& lightData = m_Lights[SelectedLightIndex];

        ImGui::Text("Position");
        SetDirty(ImGui::SliderFloat("X", &lightData.pos.x, -100.0f, 100.0f, "%0.5f"), bDirty);
        SetDirty(ImGui::SliderFloat("Y", &lightData.pos.y, -100.0f, 100.0f, "%0.5ff"), bDirty);
        SetDirty(ImGui::SliderFloat("Z", &lightData.pos.z, -100.0f, 100.0f, "%0.5f"), bDirty);

        ImGui::Text("Cone Direction / Orientation");
        SetDirty(ImGui::SliderFloat("X", &lightData.coneDir.x, -90.0f, 90.0f, "%0.5f"), bDirty);
        SetDirty(ImGui::SliderFloat("Y", &lightData.coneDir.y, -90.0f, 90.0f, "%0.5ff"), bDirty);
        SetDirty(ImGui::SliderFloat("Z", &lightData.coneDir.z, -90.0f, 90.0f, "%0.5f"), bDirty);

        SetDirty(ImGui::SliderFloat("X", &lightData.coneDir.x, -90.0f, 90.0f, "%0.5f"), bDirty);

        ImGui::Text("Radius");
        SetDirty(ImGui::SliderFloat("Z", &lightData.radiusSq, 100.0f, 2000000.0f, "%100.f"), bDirty);

        ImGui::Text("Color");
        ImGui::ColorEdit3("Light Diffuse Color", &lightData.color.x);
        // ImGui::ColorEdit3("Light Diffuse Color", &lightData.ambient[0]);

        static const char* curItem = sm_LightLabel[lightData.type];

        if (ImGui::BeginCombo("Light Type", curItem))
        {
            for (int n = 0; n < _countof(sm_LightLabel); ++n)
            {
                const bool isSelected = (curItem == sm_LightLabel[n]);

                if (ImGui::Selectable(sm_LightLabel[n], isSelected))
                {
                    curItem = sm_LightLabel[n];
                    if (curItem == sm_LightLabel[0])
                    {
                        lightData.type = 0;
                    }
                    else if (curItem == sm_LightLabel[1])
                    {
                        lightData.type = 1;
                    }
                    else
                    {
                        lightData.type = 2;
                    }
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (bDirty == true)
        {
            Camera tempCamera(nullptr, "To modify light");
            // tempCamera.SetEyeAtUp(lightData.pos, DirectX::operator+(lightData.pos, lightData.coneDir), Math::Vector3(0, 1, 0));
            tempCamera.SetEyeAtUp
            (
                lightData.pos,
                Math::Vector3(lightData.pos.x + lightData.coneDir.x, lightData.pos.y + lightData.coneDir.y, lightData.pos.z + lightData.coneDir.z),
                Math::Vector3(0, 1, 0)
            );

            const float ConeOuter = acos(lightData.coneAngles.y);
            const float lightRadius = sqrt(lightData.radiusSq);

            tempCamera.SetPerspectiveMatrix(ConeOuter * 2, 1.0f, lightRadius / 20.0f, lightRadius);
            tempCamera.Update();

            m_LightShadowMatrixes[SelectedLightIndex] = tempCamera.GetViewProjMatrix();

            Math::Matrix4 shadowTextureMatrix = Math::Matrix4(Math::AffineTransform(Math::Matrix3::MakeScale(0.5f, -0.5f, 1.0f), Math::Vector3(0.5f, 0.5f, 0.0f))) * m_LightShadowMatrixes[SelectedLightIndex];

            memcpy_s(lightData.shadowTextureMatrix, sizeof(float) * 16, &shadowTextureMatrix, sizeof(shadowTextureMatrix));

            m_DirtyLightIndex = SelectedLightIndex;
        }
        ImGui::EndChild();
    }

    ImGui::EndChild();
    */
}

void LightPrePass::CreateLight(const uint32_t LightType)
{
    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    {
        static Camera _shadowCamera(nullptr, "Create Light");

        _shadowCamera.SetEyeAtUp(Math::Vector3(-661.167603f, 1413.05164f, -584.823120f), Math::Vector3(-661.741699f, 1412.91907f, -585.631104f), Math::Vector3(0, 1, 0));
        _shadowCamera.SetPerspectiveMatrix(0.8f, 1.0f, 20.0f, 1000.0f);
        _shadowCamera.Update();

        m_LightShadowMatrixes.push_back(_shadowCamera.GetViewProjMatrix());
    }
    {
        m_Lights.push_back(LightData());

        LightData& NewLight = m_Lights.back();

        NewLight.pos.x        = -661.167603f;
        NewLight.pos.y        = 1413.05164f;
        NewLight.pos.z        = -584.823120f;
        NewLight.radiusSq     = 250000.0f;
        NewLight.color.x      = 0.5f;
        NewLight.color.y      = 0.5f;
        NewLight.color.z      = 0.5f;
        NewLight.type         = LightType;
        NewLight.coneDir.x    = -0.60f;
        NewLight.coneDir.y    = -0.15f;
        NewLight.coneDir.z    = -0.80f;
        NewLight.coneAngles.x = 50.0f;
        NewLight.coneAngles.y = 0.90f;
        memcpy_s(&NewLight.shadowTextureMatrix[0], sizeof(Math::Matrix4), &m_LightShadowMatrixes.back(), sizeof(Math::Matrix4));
        // *(Math::Matrix4*)(NewLight.shadowTextureMatrix) = m_LightShadowMatrixes.back();
    }

    recreateBuffers();
}

// ImGui::BeginPopupContextItem
void LightPrePass::CreateSphereLight()
{
    CreateLight(0u);
}

void LightPrePass::CreateConeLight()
{
    CreateLight(1u);
}

void LightPrePass::CreateConeShadowedLight()
{
    CreateLight(2u);
}

void LightPrePass::recreateBuffers()
{
    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    size_t LightDataSize = m_Lights.size();
    size_t LightShadowMatrixesSize = m_LightShadowMatrixes.size();
    ASSERT(LightDataSize == LightShadowMatrixesSize, "Light and ShadowMatrix containers' sizes are not the same.");

    sortContainer();

	{
		uint32_t lightGridSizeBytes = sm_kLightGridCells * (4 + (uint32_t)LightDataSize * 4);

		bufferManager::g_LightBuffer.CreateUAVCommitted(L"g_LightBuffer", (uint32_t)LightDataSize, sizeof(LightData), m_Lights.data());
		bufferManager::g_LightGrid.CreateUAVCommitted(L"g_LightGrid", lightGridSizeBytes, 1, nullptr);
		bufferManager::g_LightShadowArray.CreateCommittedArray(L"g_LightShadowArray", sm_kShadowBufferSize, sm_kShadowBufferSize, (uint32_t)LightDataSize, DXGI_FORMAT_R16_UNORM);
	}
}

// sort lights by type(Sphere, Cone, Cone w/ shadow Map)
// In HLSL code, to use BIT_MASKING Tehcnique, we need sorted container.
void LightPrePass::sortContainer() // LIGHT_GRID_PRELOADING - Testing HLSL 6.0 
{
    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    size_t LightDataSize = m_Lights.size();
    size_t LightShadowMatrixesSize = m_LightShadowMatrixes.size();
    ASSERT(LightDataSize == LightShadowMatrixesSize, "Light and ShadowMatrix containers' size are not the same.");

    LightData* CopyLights = new LightData[LightDataSize];
    memcpy_s(&CopyLights[0], sizeof(LightData) * LightDataSize, m_Lights.data(), sizeof(LightData) * LightDataSize);

    Math::Matrix4* CopyLightShadowMatrixes = new Math::Matrix4[LightShadowMatrixesSize];
    memcpy_s(&CopyLightShadowMatrixes[0], sizeof(Math::Matrix4) * LightShadowMatrixesSize, m_LightShadowMatrixes.data(), sizeof(Math::Matrix4) * LightShadowMatrixesSize);

    uint32_t* SortArray = new uint32_t[LightDataSize];

    for (uint32_t i = 0; i < LightDataSize; ++i)
    {
        SortArray[i] = i;
    }

    std::sort
    (
        SortArray, SortArray + LightDataSize,
        [&](const uint32_t& a, const uint32_t& b) -> bool
        {
            return m_Lights[a].type < m_Lights[b].type;
        }
    );

    for (size_t i = 0; i < LightDataSize; ++i)
    {
        m_Lights[i] = CopyLights[SortArray[i]];
        m_LightShadowMatrixes[i] = CopyLightShadowMatrixes[SortArray[i]];
    }

    for (uint32_t n = 0; n < LightDataSize; ++n)
    {
        if (m_Lights[n].type == 1)
        {
            m_FirstConeLightIndex = n;
            break;
        }
        m_FirstConeLightIndex = -1;
    }
    for (uint32_t n = 0; n < LightDataSize; ++n)
    {
        if (m_Lights[n].type == 2)
        {
            m_FirstConeShadowedLightIndex = n;
            break;
        }
        m_FirstConeLightIndex = -1;
    }

    delete[] CopyLights;
    delete[] CopyLightShadowMatrixes;
    delete[] SortArray;
}

uint32_t LightPrePass::GetFirstConeLightIndex()
{
    return m_FirstConeLightIndex;
}
uint32_t LightPrePass::GetFirstConeShadowedLightIndex()
{
    return m_FirstConeShadowedLightIndex;
}