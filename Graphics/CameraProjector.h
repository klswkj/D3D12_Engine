#pragma once
#include "stdafx.h"
#include "CameraFrustum.h"

class Camera;
enum class eObjectFilterFlag;

// SetRotation -> Math::Matrix3(Directional Vectors), XMFLOAT3(Euler Angle?????????????), Math::Quaternion

class CameraProjector
{
public:
	CameraProjector(Camera* pParent, float VerticalFOV, float AspectHeightOverWidth, float NearZClip, float FarZClip);
	DirectX::XMMATRIX GetProjectionMatrix() const;
	void RenderWidgets();
	
	void SetPosition(Math::Vector3& Position);
	void SetPosition(DirectX::XMFLOAT3& Position);

	void SetRotation(const DirectX::XMFLOAT3& RollPitchYaw);

	void Submit(eObjectFilterFlag Filter) const;
	void LinkTechniques(MasterRenderGraph& _MasterRenderGraph);
	void Reset();
private:
	float m_aspectHeightOverWidth;
	float m_width;
	float m_height;
	float m_nearZClip;
	float m_farZClip;
	float m_verticalFOV;
	float m_initAspect;
	float m_initNearZClip;
	float m_initFarZClip;
	CameraFrustum m_CameraFrustum;
	Camera* m_pParentCamera;
};

inline void CameraProjector::SetPosition(Math::Vector3& Position)
{
	m_CameraFrustum.SetPosition(Position);
}

inline void CameraProjector::SetPosition(DirectX::XMFLOAT3& Position)
{
	m_CameraFrustum.SetPosition(Position);
}

inline void CameraProjector::SetRotation(const DirectX::XMFLOAT3& RollPitchYaw)
{
	m_CameraFrustum.SetRotation(RollPitchYaw);
}
