#pragma once
#include "Entity.h"
#include "RootSignature.h"
#include "PSO.h"

#include "MathBasic.h"
#include "IBaseCamera.h"

class Camera;

class CameraFrustum final : public IEntity
{
public:
	// width, height is given -> near Z plane is normalized, -> multiplication with m_AspectRatio.
	// width / height will be VerticalFOV
	CameraFrustum(Camera& rCamera, const float aspectHeightOverWidth, const float nearZClip, const float farZClip);
	void SetVertices(const float width, const float Height, const float nearZClip, const float farZClip);
	void SetVertices(const float aspectHeightOverWidth, const float nearZClip, const float farZClip);
	void SetPosition(const DirectX::XMFLOAT3& position) noexcept;
	void SetPosition(const Math::Vector3& position) noexcept;
	void SetRotation(const DirectX::XMFLOAT3& position) noexcept;
	void SetRotation(const Math::Vector3& position) noexcept;
	Math::Matrix4 GetTransform() const noexcept final;

private:
	Math::Vector3 m_CameraPosition = { 0.0f,0.0f,0.0f };          // Convert to Orthogonal Transform
	Math::Vector3 m_Rotation = { 0.0f,0.0f,0.0f };                // Both Component

	Camera* m_pParentCamera;
	// Math::OrthogonalTransform m_CameraToWorld;

	// ���� Ȯ���ؾ� �� �� : 
	// GeneralPurpose.hlsl�� �� PSO��, RootSignature��
	// Global����� �� �� ������.
	// �ϴ� ��������� ���.
	custom::RootSignature m_RootSignature;
	GraphicsPSO m_PSO;
};
