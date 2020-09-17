#pragma once
#include "Entity.h"
#include "RootSignature.h"
#include "CommandContext.h"

#include "MathBasic.h"
#include "IBaseCamera.h"

class Camera;

class CameraFrustum : public Entity
{
public:
	// width, height is given -> near Z plane is normalized, -> multiplication with m_AspectRatio.
	// width / height will be VerticalFOV
	CameraFrustum(Camera* pCamera, float AspectHeightOverWidth, float NearZClip, float FarZClip);
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
