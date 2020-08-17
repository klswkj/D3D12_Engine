#pragma once
#include "Entity.h"
#include "RootSignature.h"
#include "PSO.h"
#include "MathBasic.h"

// CameraIndicator's D3D_PRIMITIVE_TOPLOGY is LINELIST!
class CameraEntity : public Entity
{
public:
	CameraEntity();
	void SetPosition(DirectX::XMFLOAT3& Position) noexcept;
	void SetPosition(Math::Vector3& Position) noexcept;
	void SetRotation(DirectX::XMFLOAT3& RollPitchYaw) noexcept;
	void SetRotation(Math::Quaternion& _Quaternion) noexcept;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;

private:
	Math::Vector3 m_CameraPosition{ 0.0f,0.0f,0.0f };
	Math::Vector3 m_Rotation{ 0.0f,0.0f,0.0f };

	Math::OrthogonalTransform m_CameraToWorld; // Camera::IBaseCamera에도 똑같이 있음.

	// 현재 확인해야 할 것 : 
	// GeneralPurpose.hlsl를 쓸 PSO와, RootSignature를
	// Global밸류로 쓸 수 있을지.
	// 일단 멤버변수로 사용.
	custom::RootSignature m_RootSignature;
	GraphicsPSO m_PSO;
};

inline void CameraEntity::SetPosition(DirectX::XMFLOAT3& Position) noexcept
{
	m_CameraPosition = Position;
	m_CameraToWorld.SetTranslation(Position);
}

inline void CameraEntity::SetPosition(Math::Vector3& Position) noexcept
{
	m_CameraPosition = Position;
	this->m_CameraToWorld.SetTranslation(Position);
}

inline void CameraEntity::SetRotation(DirectX::XMFLOAT3& RollPitchYaw) noexcept
{
	m_Rotation = RollPitchYaw;
	m_CameraToWorld.SetRotation(RollPitchYaw);
}

inline void CameraEntity::SetRotation(Math::Quaternion& _Quaternion) noexcept
{
	// m_Rotation = DirectX::XMRotation -> Quaternion convert to EulerAngle... Gimber lock
	m_CameraToWorld.SetRotation(_Quaternion);
}

inline DirectX::XMMATRIX CameraEntity::GetTransformXM() const noexcept
{
	return DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMVECTOR(m_Rotation)) *
		DirectX::XMMatrixTranslationFromVector(DirectX::XMVECTOR(m_CameraPosition));
}
