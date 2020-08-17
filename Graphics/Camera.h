#pragma once
#include "IBaseCamera.h"
#include "CameraEntity.h"
#include "CameraProjector.h"

enum class eObjectFilterFlag;

#ifndef BOOL
#define BOOL int
#endif

// Camera ─┬─ CameraProjector ── CameraFrustum 
//         └  CameraEntity (the thing that made of line list.)

// TODO : LinkTechniques(), Submit(), GetName() 구현
class Camera : public IBaseCamera
{
public:
	using IBaseCamera::IBaseCamera;

	Camera();

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
	D3D11 void SetPitch(float _Pitch) { m_Pitch = _Pitch; }
	D3D11 void SetYaw(float _Yaw) { m_Yaw = _Yaw; }
	D3D11 float GetPitch() const { return m_Pitch; }
	D3D11 float GetYaw() const { return m_Yaw; }
	D3D11 void Reset();
	D3D11 void Submit(eObjectFilterFlag Filter);

	// D3D11 void BindToCommandContext(); -> Camera가 빛으로 쓰일 수 있으니까 지금은 패스.
	D3D11 void RenderWindow() noexcept;
	D3D11 const char* GetName() const noexcept;

protected:
	void UpdateProjMatrix();

protected:
	CameraEntity m_Entity;
	CameraProjector m_Projector;

	float m_VerticalFOV; // Field of view angle in radians
	float m_AspectRatio;
	float m_NearZClip;
	float m_FarZClip;

	Math::Vector3 m_Position;
	float m_Pitch; // -> m_up      ???
	float m_Yaw;   // -> m_right   ???
	               // -> m_forward ??? 

	Math::Vector3 m_InitPosition; // Reset Coordinate
	//float InitRoll; // Fan  ( +되면 시계방향)
	float m_InitPitch;  // 상하 ( +되면 아래로 숙임)
	float m_InitYaw;   // 좌우  ( +되면 오른쪽으로)

    bool m_bTethered;
    bool m_bDrawCamera{ true };
    bool m_bDrawProjector{ true };
    const char* m_Name; // -> TODO : Convert To FastName.
};

inline Camera::Camera()// : m_ReverseZ(true)
	: m_Projector(this, DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f)
{
	SetPerspectiveMatrix(DirectX::XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f); // Camera Frustum
}

inline void Camera::SetPerspectiveMatrix(float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip)
{
	m_VerticalFOV = verticalFovRadians;
	m_AspectRatio = aspectHeightOverWidth;
	m_NearZClip = nearZClip;
	m_FarZClip = farZClip;

	UpdateProjMatrix();

	m_PreviousViewProjMatrix = m_ViewProjMatrix;
}

#ifdef BOOL
#pragma pop_macro("BOOL")
#endif