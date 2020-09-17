#include "stdafx.h"
#include "Camera.h"
#include "Vector.h"
#include "CameraEntity.h"
#include "CameraFrustum.h"

Camera::Camera(char* Name, Math::Vector3 InitPosition, float InitPitch, float InitYaw)
	: m_Name(Name), m_InitPosition(InitPosition), m_InitPitch(InitPitch), m_InitYaw(InitYaw),
	m_Projector(this, DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f)
{
	SetPerspectiveMatrix(DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f); // Camera Frustum

	Reset();
}

Camera::Camera(const char* Name, Math::Vector3 InitPosition/* = Math::Vector3{ 0.0f, 0.0f, 0.0f }*/, float InitPitch/* = 0.0f*/, float InitYaw/* = 0.0f*/)
	: m_Name(Name), m_InitPosition(InitPosition), m_InitPitch(InitPitch), m_InitYaw(InitYaw),
	m_Projector(this, DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f)
{
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
	bool bRotationDirty{ false };
	bool bPositionDirty{ false };
	const auto dcheck = [](bool d, bool& carry) { carry = carry || d; };

	ImGui::Text(m_Name.c_str());

	if (!m_bTethered)
	{
		ImGui::Text("Position");

		Math::Vector3* temp = &m_CameraToWorld.GetTranslation();

		// Warning 항상의심.
		dcheck(ImGui::SliderFloat("X", &temp->m_vec.m128_f32[0], -80.0f, 80.0f, "%.1f"), bPositionDirty); // ViewMatrix
		dcheck(ImGui::SliderFloat("Y", &temp->m_vec.m128_f32[1], -80.0f, 80.0f, "%.1f"), bPositionDirty); // ViewMatrix
		dcheck(ImGui::SliderFloat("Z", &temp->m_vec.m128_f32[2], -80.0f, 80.0f, "%.1f"), bPositionDirty); // ViewMatrix
	}

	ImGui::Text("Orientation");
	dcheck(ImGui::SliderAngle("Pitch", &m_Pitch, -90.0f, 90.0f), bRotationDirty); // Show Radian Value.
	dcheck(ImGui::SliderAngle("Yaw", &m_Yaw, -180.0f, 180.0f), bRotationDirty);   // Show Radian Value.

	m_Projector.RenderWidgets();

	ImGui::Checkbox("Camera Indicator", &m_bDrawCamera);
	ImGui::Checkbox("Frustum Indicator", &m_bDrawProjector);

	if (ImGui::Button("Reset"))
	{
		Reset();
	}

	if (bPositionDirty)
	{
		Math::Vector3 Position = m_CameraToWorld.GetTranslation();
		m_Entity.SetPosition(Position);
		m_Projector.SetPosition(Position);
	}

	if (bRotationDirty)
	{
		DirectX::XMFLOAT3 PitchYaw = { m_Pitch, m_Yaw, 0.0f };
		m_Entity.SetRotation(PitchYaw);
		m_Projector.SetRotation(PitchYaw);
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