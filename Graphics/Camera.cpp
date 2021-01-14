#include "stdafx.h"
#include "Camera.h"
#include "CameraManager.h"
#include "CameraEntity.h"
#include "CameraFrustum.h"
#include "CustomImgui.h"

Camera::Camera(CameraManager* OwningManager, char* Name, bool Visible, Math::Vector3 InitPosition, float InitPitch, float InitYaw)
	: m_Projector(this, DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f),
	m_Entity(this)
{
	m_Name         = Name;
	m_InitPosition = InitPosition;
	m_InitPitch    = m_InitPitch;
	m_InitYaw      = m_InitYaw;

	m_bTethered      = false;
	m_bDrawCamera    = Visible;
	m_bDrawProjector = Visible;

	m_pCameraManager = OwningManager;

	SetPerspectiveMatrix(DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f); // Camera Frustum

	Reset();
}

Camera::Camera(CameraManager* OwningManager, const char* Name, bool Visible, Math::Vector3 InitPosition/* = Math::Vector3{ 0.0f, 0.0f, 0.0f }*/, float InitPitch/* = 0.0f*/, float InitYaw/* = 0.0f*/)
	: m_Projector(this, DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f),
	m_Entity(this)
{
	m_Name = Name;
	m_InitPosition = InitPosition;
	m_InitPitch = m_InitPitch;
	m_InitYaw = m_InitYaw;

	m_bTethered      = false;
	m_bDrawCamera    = Visible;
	m_bDrawProjector = Visible;

	m_pCameraManager = OwningManager;

	SetPerspectiveMatrix(DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f); // Camera Frustum

	Reset();
}

void Camera::UpdateProjMatrix()
{
    float Y = 1.0f / std::tanf(m_VerticalFOV * 0.5f); // Y = 2 * (NearZ / Width)
    float X = Y * m_AspectRatio; // X = 2 * (NearZ / Height)

    float Q1, Q2;

    // ReverseZ puts far plane at Z=0 and near plane at Z=1.  This is never a bad idea, and it's
    // actually a great idea with F32 depth buffers to redistribute precision more evenly across
    // the entire range.  It requires clearing Z to 0.0f and using a GREATER variant depth test.
    // Some care must also be done to properly reconstruct linear W in a pixel shader from hyperbolic Z.
    Q1 = m_NearZClip / (m_FarZClip - m_NearZClip);
    Q2 = Q1 * m_FarZClip;
    
    /*
    if (m_ReverseZ)
    {
        Q1 = m_NearZClip / (m_FarZClip - m_NearZClip);
        Q2 = Q1 * m_FarZClip;
    }
    else
    {
        Q1 = m_FarZClip / (m_NearZClip - m_FarZClip);
        Q2 = Q1 * m_NearZClip;
    }
    */
	SetProjMatrix
	(
		Math::Matrix4
        (
			Math::Vector4(X, 0.0f, 0.0f, 0.0f),
			Math::Vector4(0.0f, Y, 0.0f, 0.0f),
			Math::Vector4(0.0f, 0.0f, Q1, -1.0f),
			Math::Vector4(0.0f, 0.0f, Q2, 0.0f)
		)
	);
}

void Camera::RenderWindow() noexcept
{
	if (!m_pCameraManager)
	{
		return;
	}

	bool bRotationDirty{ false };
	bool bPositionDirty{ false };
	const auto dcheck = [](bool d, bool& carry) { carry = carry || d; };

	ImGui::CenteredSeparator(100);
	ImGui::SameLine();
	ImGui::CenteredSeparator();
	ImGui::Text(m_Name.c_str());

	const CameraManager::CaptureBox& CaptureBox = m_pCameraManager->GetCaputreBox();
	Math::Vector3 MinPoint = CaptureBox.MinPoint;
	Math::Vector3 MaxPoint = CaptureBox.MaxPoint;

	if (!m_bTethered)
	{
		ImGui::Text("Position");

		Math::Vector3 temp = m_CameraToWorld.GetTranslation();

		dcheck(ImGui::SliderFloat("X", &temp.m_vec.m128_f32[0], MinPoint.GetX(), MaxPoint.GetX(), "%.1f"), bPositionDirty); // ViewMatrix
		dcheck(ImGui::SliderFloat("Y", &temp.m_vec.m128_f32[1], MinPoint.GetY(), MaxPoint.GetY(), "%.1f"), bPositionDirty); // ViewMatrix
		dcheck(ImGui::SliderFloat("Z", &temp.m_vec.m128_f32[2], MinPoint.GetZ(), MaxPoint.GetZ(), "%.1f"), bPositionDirty); // ViewMatrix

		if (bPositionDirty)
		{
			SetPosition(temp);

			Math::Vector3 Position = m_CameraToWorld.GetTranslation();
			m_Entity.SetPosition(Position);
			m_Projector.SetPosition(Position);
		}

		ImGui::Text("Orientation");

		float PitchTemp = m_pCameraManager->GetCurrentPitch();
		float YawTemp   = m_pCameraManager->GetCurrentHeading();

		dcheck(ImGui::SliderAngle("Pitch", &PitchTemp, -90.0f, 90.0f), bRotationDirty); // Show Radian Value.
		dcheck(ImGui::SliderAngle("Yaw",   &YawTemp, -180.0f, 180.0f), bRotationDirty);   // Show Radian Value.

		if (bRotationDirty)
		{
			m_Pitch = PitchTemp;
			m_Yaw   = YawTemp;
			m_pCameraManager->SetCurrentPitch(PitchTemp);
			m_pCameraManager->SetCurrentHeading(YawTemp);

			DirectX::XMFLOAT3 PitchYaw = { m_Pitch, m_Yaw, 0.0f };
			m_Entity.SetRotation(PitchYaw);
			m_Projector.SetRotation(PitchYaw);
		}
	}

	ImGui::CenteredSeparator(100);
	ImGui::SameLine();
	ImGui::CenteredSeparator();

	m_Projector.RenderWidgets();

	ImGui::Checkbox("Camera Indicator", &m_bDrawCamera);
	ImGui::Checkbox("Frustum Indicator", &m_bDrawProjector);

	if (ImGui::Button("Reset"))
	{
		Reset();
	}
}

void Camera::Submit(eObjectFilterFlag Filter)
{
	if (m_bDrawCamera)
	{
		m_Entity.Submit(Filter);
	}
	if (m_bDrawProjector)
	{
		m_Projector.Submit(Filter);
	}
}

void Camera::Reset()
{
	// m_CameraToWorld.SetTranslation(m_InitPosition);
	SetPosition(m_InitPosition);
	m_Entity.SetPosition(m_InitPosition);
	m_Projector.SetPosition(m_InitPosition);
}

std::string& Camera::GetName() noexcept
{
    return m_Name;
}

void Camera::LinkTechniques(MasterRenderGraph& _MasterRenderGraph)
{
	m_Entity.LinkTechniques(_MasterRenderGraph);
	m_Projector.LinkTechniques(_MasterRenderGraph);
}