#include "stdafx.h"
#include "LightPrePass.h"
#include "Matrix4.h"
#include "UAVBuffer.h"
#include "ColorBuffer.h"
#include "BufferManager.h"
#include "ShadowBuffer.h"
#include "PSO.h"
#include "RootSignature.h"
#include "CommandContext.h"
#include "ComputeContext.h"
#include "IBaseCamera.h"
#include "Camera.h"
#include "ShaderConstantsTypeDefinitions.h"

uint32_t LightPrePass::sm_MaxLight = 128u;
const char* LightPrePass::sm_LightLabel[3] = { "Sphere Light", "Cone Light", "Cone Light with ShadowMap" };

/*
struct LightData
{
	float pos[3];
	float radiusSq;
	float color[3];

	uint32_t type;

	float coneDir[3];
	float coneAngles[2];

	float shadowTextureMatrix[16];
};

struct CSConstants
{
    uint32_t ViewportWidth, ViewportHeight;
    float InvTileDim;
    float RcpZMagic;
    uint32_t TileCount;
    Math::Matrix4 ViewProjMatrix;
    // float ViewPorjMatrx[16];
};
*/
LightPrePass::LightPrePass(std::string pName)
    : Pass(pName)
{
    bufferManager::g_Lights.reserve(sm_MaxLight * 2 + 1);
    bufferManager::g_LightShadowMatrixes.reserve(sm_MaxLight * 2 + 1);

    if (bufferManager::g_Lights.size() || bufferManager::g_LightShadowMatrixes.size())
    {
        recreateBuffers();
    }
}

LightPrePass::~LightPrePass()
{
    bufferManager::g_LightBuffer.Destroy();
    bufferManager::g_LightGrid.Destroy();
    bufferManager::g_LightGridBitMask.Destroy();
    bufferManager::g_LightShadowArray.Destroy();
    bufferManager::g_CumulativeShadowBuffer.Destroy();
    bufferManager::g_Lights.clear();
    bufferManager::g_LightShadowMatrixes.clear();
}

void LightPrePass::Execute(custom::CommandContext& BaseContext)
{
    
}

void LightPrePass::RenderWindow() DEBUG_EXCEPT
{
    ImGui::Begin("LightPrePass"); // 여기 Begin말고, ImGui::BeginChild해야, 

    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    ASSERT(m_Lights.size() == m_LightShadowMatrixes.size());

    const uint32_t bHasLight = (m_Lights.size() && m_LightShadowMatrixes.size());

    ImGui::Columns(2 + bHasLight, nullptr, true);
    {
        ImGui::BeginChild("", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

        if (m_Lights.size() < sm_MaxLight)
        {
            bool bDirty = false;

            if (ImGui::Button("Create Sphere Light"))
            {
                CreateSphereLight();
                bDirty = true;
            }
            if (ImGui::Button("Create Cone Light"))
            {
                CreateConeLight();
                bDirty = true;
            }

            if (bDirty)
            {
                recreateBuffers();
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
    }

    static size_t SelectedLightIndex = -1;
    {
        ImGui::BeginChild("", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

        const int Imgui_Node_Flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_FramePadding |
            ImGuiTreeNodeFlags_Leaf;

        for (size_t i= 0; i < m_Lights.size(); ++i)
        {
            ImGui::TreeNodeEx("Light", Imgui_Node_Flags, sm_LightLabel[m_Lights[i].type]);
            
            if (ImGui::IsItemClicked())
            {
                SelectedLightIndex = i;
            }
        }

        ImGui::EndChild();
        ImGui::NextColumn();
    }

    if (bHasLight && SelectedLightIndex != -1)
    {
        ImGui::BeginChild("", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);

        bool dirtyPos = false;
        bool dirtyCone = false;
        bool dirtyRadius = false;
        bool dirtyColor = false;
        bool bTypeChanged = false;
        const auto SetDirty = [](bool given, bool dirtybit) {dirtybit = dirtybit || given; };

        LightData& lightData = m_Lights[SelectedLightIndex];

        ImGui::Text("Position");
        SetDirty(ImGui::SliderFloat("X", &lightData.pos.x, -100.0f, 100.0f, "%0.5f"), dirtyPos);
        SetDirty(ImGui::SliderFloat("Y", &lightData.pos.y, -100.0f, 100.0f, "%0.5ff"), dirtyPos);
        SetDirty(ImGui::SliderFloat("Z", &lightData.pos.z, -100.0f, 100.0f, "%0.5f"), dirtyPos);

        ImGui::Text("Cone Direction / Orientation");
        SetDirty(ImGui::SliderFloat("X", &lightData.coneDir.x, -90.0f, 90.0f, "%0.5f"), dirtyCone);
        SetDirty(ImGui::SliderFloat("Y", &lightData.coneDir.y, -90.0f, 90.0f, "%0.5ff"), dirtyCone);
        SetDirty(ImGui::SliderFloat("Z", &lightData.coneDir.z, -90.0f, 90.0f, "%0.5f"), dirtyCone);

        SetDirty(ImGui::SliderFloat("X", &lightData.coneDir.x, -90.0f, 90.0f, "%0.5f"), dirtyCone);

        ImGui::Text("Radius");
        SetDirty(ImGui::SliderFloat("Z", &lightData.radiusSq, 100.0f, 2000000.0f, "%100.f"), dirtyRadius);

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
                    bTypeChanged = true;
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
            ImGui::EndChild();
        }

        Camera tempCamera("To modify light");
        // tempCamera.SetEyeAtUp(lightData.pos, DirectX::operator+(lightData.pos, lightData.coneDir), Math::Vector3(0, 1, 0));
        tempCamera.SetEyeAtUp(lightData.pos, 
            Math::Vector3(lightData.pos.x + lightData.coneDir.x, lightData.pos.y + lightData.coneDir.y, lightData.pos.z + lightData.coneDir.z), 
            Math::Vector3(0, 1, 0));

        const float ConeOuter = acos(lightData.coneAngles.y);
        const float lightRadius = sqrt(lightData.radiusSq);

        tempCamera.SetPerspectiveMatrix(ConeOuter * 2, 1.0f, lightRadius / 20.0f, lightRadius);
        tempCamera.Update();

        m_LightShadowMatrixes[SelectedLightIndex] = tempCamera.GetViewProjMatrix();

        Math::Matrix4 shadowTextureMatrix = Math::Matrix4(Math::AffineTransform(Math::Matrix3::MakeScale(0.5f, -0.5f, 1.0f), Math::Vector3(0.5f, 0.5f, 0.0f))) * m_LightShadowMatrixes[SelectedLightIndex];

        memcpy_s(lightData.shadowTextureMatrix, sizeof(float) * 16, &shadowTextureMatrix, sizeof(shadowTextureMatrix));

        m_DirtyLightIndex = SelectedLightIndex;
    }
    ImGui::End(); // ImGui::EndChild() 만들어야할 수도
}

// ImGui::BeginPopupContextItem
void LightPrePass::CreateSphereLight()
{
    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    {
        static Camera shadowCamera("To Create Sphere Light");

        shadowCamera.SetEyeAtUp(Math::Vector3(0, 0, 0), Math::Vector3(0, 1, 0), Math::Vector3(0, 1, 0));
        shadowCamera.SetPerspectiveMatrix(0.8f, 1.0f, 20.0f, 1000.0f);
        shadowCamera.Update();

        m_LightShadowMatrixes.push_back(shadowCamera.GetViewProjMatrix());
    }
	{
		m_Lights.push_back(LightData());

		LightData& NewLight = m_Lights.back();

		NewLight.pos.x = 0.0f;
		NewLight.pos.y = 0.0f;
		NewLight.pos.z = 0.0f;
		NewLight.radiusSq = 250000.0f;
		NewLight.color.x = 0.5f;
		NewLight.color.y = 0.5f;
		NewLight.color.z = 0.5f;
		NewLight.type = 0u;
		NewLight.coneDir.x = -0.60f;
		NewLight.coneDir.y = -0.15f;
		NewLight.coneDir.z = -0.80f;
		NewLight.coneAngles.x = 50.0f;
		NewLight.coneAngles.y = 0.90f;
        memcpy_s(&NewLight.shadowTextureMatrix[0], sizeof(Math::Matrix4), &m_LightShadowMatrixes.back(), sizeof(Math::Matrix4));
        // *(Math::Matrix4*)(NewLight.shadowTextureMatrix) = m_LightShadowMatrixes.back();
	}

    // In HLSL code, to use BIT_MASKING Tehcnique,  we need sort containers. 
    sortContainers();
}

void LightPrePass::CreateConeLight()
{
    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    {
        static Camera shadowCamera("To Create Cone Light");

        shadowCamera.SetEyeAtUp(Math::Vector3(0, 0, 0), Math::Vector3(0, 1, 0), Math::Vector3(0, 1, 0));
        shadowCamera.SetPerspectiveMatrix(0.8f, 1.0f, 20.0f, 1000.0f);
        shadowCamera.Update();

        m_LightShadowMatrixes.push_back(shadowCamera.GetViewProjMatrix());
    }
    {
        m_Lights.push_back(LightData());

        LightData& NewLight = m_Lights.back();

        NewLight.pos.x = 0.0f;
        NewLight.pos.y = 0.0f;
        NewLight.pos.z = 0.0f;
        NewLight.radiusSq = 250000.0f;
        NewLight.color.x = 0.5f;
        NewLight.color.y = 0.5f;
        NewLight.color.z = 0.5f;
        NewLight.type = 1u;
        NewLight.coneDir.x = -0.60f;
        NewLight.coneDir.y = -0.15f;
        NewLight.coneDir.z = -0.80f;
        NewLight.coneAngles.x = 50.0f;
        NewLight.coneAngles.y = 0.90f;
        memcpy_s(&NewLight.shadowTextureMatrix[0], sizeof(Math::Matrix4), &m_LightShadowMatrixes.back(), sizeof(Math::Matrix4));
        // *(Math::Matrix4*)(NewLight.shadowTextureMatrix) = m_LightShadowMatrixes.back();
    }

    // In HLSL code, to use BIT_MASKING Tehcnique,  we need sorted containers. 
    sortContainers();
}

void LightPrePass::recreateBuffers()
{
    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    size_t LightDataSize = m_Lights.size();
    size_t LightShadowMatrixesSize = m_LightShadowMatrixes.size();
    ASSERT(LightDataSize == LightShadowMatrixesSize, "Light and ShadowMatrix containers' sizes are not the same.");

    for (uint32_t n = 0; n < sm_MaxLight; n++)
    {
        if (m_Lights[n].type == 1)
        {
            m_FirstConeLightIndex = n;
            break;
        }
        m_FirstConeLightIndex = -1;
    }
    for (uint32_t n = 0; n < sm_MaxLight; n++)
    {
        if (m_Lights[n].type == 2)
        {
            m_FirstConeShadowedLightIndex = n;
            break;
        }
        m_FirstConeLightIndex = -1;
    }

    bufferManager::g_LightBuffer.Destroy();
    bufferManager::g_LightGrid.Destroy();
    bufferManager::g_LightGridBitMask.Destroy();
    bufferManager::g_CumulativeShadowBuffer.Destroy();

    bufferManager::g_LightBuffer.Create(L"m_LightBuffer", LightDataSize, sizeof(LightData), m_Lights.data());

    // todo: assumes max resolution of 1920x1080
    uint32_t lightGridCells = Math::DivideByMultiple(1920, m_kMinWorkGroupSize) * Math::DivideByMultiple(1080, m_kMinWorkGroupSize);
    uint32_t lightGridSizeBytes = lightGridCells * (4 + LightDataSize * 4);
    bufferManager::g_LightGrid.Create(L"m_LightGrid", lightGridSizeBytes, 1, nullptr);

    uint32_t lightGridBitMaskSizeBytes = lightGridCells * 4 * 4;
    bufferManager::g_LightGridBitMask.Create(L"m_LightGridBitMask", lightGridBitMaskSizeBytes, 1, nullptr);

    bufferManager::g_LightShadowArray.CreateArray(L"m_LightShadowArray", m_kShadowBufferSize, m_kShadowBufferSize, LightDataSize, DXGI_FORMAT_R16_UNORM);
    bufferManager::g_CumulativeShadowBuffer.Create(L"m_LightShadowTempBuffer", m_kShadowBufferSize, m_kShadowBufferSize);
}

// sort lights by type(Sphere, Cone, Cone w/ shadow Map)
void LightPrePass::sortContainers()
{
    std::vector<LightData>& m_Lights = bufferManager::g_Lights;
    std::vector<Math::Matrix4>& m_LightShadowMatrixes = bufferManager::g_LightShadowMatrixes;

    size_t LightDataSize = m_Lights.size();
    size_t LightShadowMatrixesSize = m_LightShadowMatrixes.size();
    ASSERT(LightDataSize == LightShadowMatrixesSize, "Light and ShadowMatrix containers' size are not the same.");

    LightData* CopyLights = (LightData*)malloc(sizeof(LightData) * LightDataSize);
    memcpy_s(&CopyLights[0], sizeof(LightData) * LightDataSize, m_Lights.data(), sizeof(LightData) * LightDataSize);

    Math::Matrix4* CopyLightShadowMatrixes = (Math::Matrix4*)malloc(sizeof(Math::Matrix4*) * (LightShadowMatrixesSize));
    memcpy_s(&CopyLightShadowMatrixes[0], sizeof(Math::Matrix4) * LightShadowMatrixesSize, m_Lights.data(), sizeof(Math::Matrix4) * LightShadowMatrixesSize);

    uint32_t* SortArray = (uint32_t*)malloc(sizeof(uint32_t) * LightDataSize);

    for (uint32_t i= 0; i < LightDataSize; ++i)
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

    for (size_t i= 0; i < LightDataSize; ++i)
    {
        m_Lights[i] = CopyLights[SortArray[i]];
        m_LightShadowMatrixes[i] = CopyLightShadowMatrixes[SortArray[i]];
    }

    delete[] CopyLights;
    delete[] CopyLightShadowMatrixes;
    delete[] SortArray;
}