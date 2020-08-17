#pragma once
#include "Entity.h"
#include "RootSignature.h"
#include "CommandContext.h"
#include "MathBasic.h"

class CameraFrustum : public Entity
{
public:
	// width, height is given -> near Z plane is normalized, -> multiplication with m_AspectRatio.
	// width / height will be VerticalFOV
	CameraFrustum(Camera pParent, float AspectHeightOverWidth, float NearZClip, float FarZClip);
	void SetVertices(float Width, float Height, float NearZClip, float FarZClip);
	void SetVertices(float AspectHeightOverWidth, float NearZClip, float FarZClip);
	void SetPosition(DirectX::XMFLOAT3& Position) noexcept;
	void SetPosition(Math::Vector3& Position) noexcept;
	void SetRotation(const DirectX::XMFLOAT3& Position) noexcept;
	void SetRotation(Math::Vector3& Position) noexcept;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	Math::Matrix4 GetTransform() const noexcept;

private:
	Math::Vector3 m_CameraPosition = { 0.0f,0.0f,0.0f };          // Convert to Orthogonal Transform
	Math::Vector3 m_Rotation = { 0.0f,0.0f,0.0f };                // Both Component

	Camera* m_pParentCamera;
	// Math::OrthogonalTransform m_CameraToWorld;

	// 현재 확인해야 할 것 : 
	// GeneralPurpose.hlsl를 쓸 PSO와, RootSignature를
	// Global밸류로 쓸 수 있을지.
	// 일단 멤버변수로 사용.
	custom::RootSignature m_RootSignature;
	GraphicsPSO m_PSO;
};

inline void CameraFrustum::SetPosition(DirectX::XMFLOAT3& Position) noexcept
{
	m_CameraPosition = Position;
	// m_CameraToWorld.SetTranslation(Position);
}

inline void CameraFrustum::SetPosition(Math::Vector3& Translation) noexcept
{
	m_CameraPosition = Translation;
	// m_CameraToWorld.SetTranslation(Translation);
}

inline void CameraFrustum::SetRotation(const DirectX::XMFLOAT3& RollPitchYaw) noexcept
{
	m_Rotation = RollPitchYaw;
	// m_CameraToWorld.SetRotation(RollPitchYaw);
}

inline void CameraFrustum::SetRotation(Math::Vector3& Rotation) noexcept
{
	m_Rotation = Rotation;
}

inline DirectX::XMMATRIX CameraFrustum::GetTransformXM() const noexcept
{
	return DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMVECTOR(m_Rotation)) *
		DirectX::XMMatrixTranslationFromVector(DirectX::XMVECTOR(m_CameraPosition));

	// return DirectX::XMMatrixRotationRollPitchYawFromVector(m_CameraToWorld.GetRotation()) *
	// 	 DirectX::XMMatrixTranslationFromVector(m_CameraToWorld.GetTranslation());

	//inline Math::Matrix4 CameraFrustum::GetTransform() const noexcept
	//{
	//	return Math::Matrix4(m_pParentCamera->GetRightUpForwardMatrix(), m_pParentCamera->GetPosition());
	//}
}

inline Math::Matrix4 CameraFrustum::GetTransform() const noexcept
{
	return Math::Matrix4(m_pParentCamera->GetRightUpForwardMatrix(), m_pParentCamera->GetPosition());
}