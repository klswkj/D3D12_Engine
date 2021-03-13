#pragma once
#include "stdafx.h"
#include "CameraFrustum.h"

class Camera;
enum class eObjectFilterFlag;

// SetRotation -> Math::Matrix3(Directional Vectors), XMFLOAT3(Euler Angle?????????????), Math::Quaternion

class CameraProjector
{
public:
	CameraProjector(Camera& rParent, const float verticalFOV, const float aspectHeightOverWidth, const float nearZClip, const float farZClip);
	DirectX::XMMATRIX GetProjectionMatrix() const;
	void RenderWidgets();
	
	void SetPosition(const Math::Vector3& position);
	void SetPosition(const DirectX::XMFLOAT3& position);

	void SetRotation(const DirectX::XMFLOAT3& rollPitchYaw);

	void Submit(const eObjectFilterFlag filter);
	void LinkTechniques(const MasterRenderGraph& masterRG);
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
	Camera* const m_pParentCamera;
};

inline void CameraProjector::SetPosition(const Math::Vector3& position)
{
	m_CameraFrustum.SetPosition(position);
}

inline void CameraProjector::SetPosition(const DirectX::XMFLOAT3& position)
{
	m_CameraFrustum.SetPosition(position);
}

inline void CameraProjector::SetRotation(const DirectX::XMFLOAT3& rollPitchYaw)
{
	m_CameraFrustum.SetRotation(rollPitchYaw);
}
