#pragma once
#include "IBaseCamera.h"
#include "CameraEntity.h"
#include "CameraProjector.h"

enum class eObjectFilterFlag;
class MasterRenderGraph;
class CameraManager;

// Camera ─┬─ CameraProjector ── CameraFrustum 
//         └  CameraEntity (the thing that made of line list.)

class Camera : public IBaseCamera
{
public:
	using IBaseCamera::IBaseCamera;

	Camera(CameraManager* OwningManager = nullptr, char* Name = nullptr, bool Visible = false, Math::Vector3 InitPosition = Math::Vector3{ 0.0f, 0.0f, 0.0f }, float InitPitch = 0.0f, float InitYaw = 0.0f);
	Camera(CameraManager* OwningManager = nullptr, const char* Name = nullptr, bool Visible = false, Math::Vector3 InitPosition = Math::Vector3{ 0.0f, 0.0f, 0.0f }, float InitPitch = 0.0f, float InitYaw = 0.0f);

	// Controls the view-to-projection matrix
	void SetPerspectiveMatrix(float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip);

	void SetFOV(float verticalFovInRadians) { m_VerticalFOV = verticalFovInRadians; UpdateProjMatrix(); }
	void SetAspectRatio(float heightOverWidth) { m_AspectRatio = heightOverWidth; UpdateProjMatrix(); }
	void SetZRange(float nearZ, float farZ) { m_NearZClip = nearZ; m_FarZClip = farZ; UpdateProjMatrix(); }
	// void ReverseZ(bool enable) { m_ReverseZ = enable; UpdateProjMatrix(); } // NotUsed.

	float GetFOV() const { return m_VerticalFOV; }
	float GetNearZClip() const { return m_NearZClip; }
	float GetFarZClip() const { return m_FarZClip; }
	float GetClearDepth() const { /*return m_ReverseZ ? 0.0f : 1.0f;*/ return 0.0f; }

	//////////////////////////////////////////////////
	//////////////////////////////////////////////////
	void SetPitch(float _Pitch) { m_Pitch = _Pitch; }
	void SetYaw(float _Yaw) { m_Yaw = _Yaw; }
	float GetPitch() const { return m_Pitch; }
	float GetYaw() const { return m_Yaw; }
	void Reset();
	void Submit(eObjectFilterFlag Filter);
	void LinkTechniques(MasterRenderGraph& _RenderGraph);

	void RenderWindow() noexcept;
	std::string& GetName() noexcept;

protected:
	void UpdateProjMatrix();

protected:
	std::string m_Name;
	CameraEntity m_Entity;
	CameraProjector m_Projector;
	CameraManager* m_pCameraManager;

	float m_VerticalFOV; // Field of view angle in radians
	float m_AspectRatio;
	float m_NearZClip;
	float m_FarZClip;

	Math::Vector3 m_Position;
	float m_Pitch;
	float m_Yaw;

	Math::Vector3 m_InitPosition; // Reset Coordinate
	//float InitRoll;  // Fan  ( +되면 시계방향)
	float m_InitPitch; // 상하 ( +되면 아래로 숙임)
	float m_InitYaw;   // 좌우 ( +되면 오른쪽으로)

	bool m_bTethered;
	bool m_bDrawCamera;
	bool m_bDrawProjector;
};

inline void Camera::SetPerspectiveMatrix(float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip)
{
	m_VerticalFOV = verticalFovRadians;
	m_AspectRatio = aspectHeightOverWidth;
	m_NearZClip = nearZClip;
	m_FarZClip = farZClip;

	UpdateProjMatrix();

	m_PreviousViewProjMatrix = m_ViewProjMatrix;
}