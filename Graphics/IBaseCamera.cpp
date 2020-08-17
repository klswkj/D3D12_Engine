#include "IBaseCamera.h"
#include "Quaternion.h"

void IBaseCamera::Update()
{
	m_PreviousViewProjMatrix = m_ViewProjMatrix;

	const Math::Matrix4 ViewMatrix = Math::Matrix4(~m_CameraToWorld);

	m_ViewProjMatrix = m_ProjMatrix * ViewMatrix;
	// m_ViewProjMatrix = DirectX::XMMatrixLookToLH
	// (
	//     DirectX::XMVECTOR(position), DirectX::XMVECTOR(orientation.GetZ()), DirectX::XMVECTOR(orientation.GetY())
	// );
	
	m_ReprojectMatrix = m_PreviousViewProjMatrix * Invert(GetViewProjMatrix());
	m_FrustumVS = Math::Frustum(m_ProjMatrix);
	m_FrustumWS = m_CameraToWorld * m_FrustumVS;
}

// Public functions for controlling where the camera is and its orientation
void IBaseCamera::SetEyeAtUp(Math::Vector3 eye, Math::Vector3 at, Math::Vector3 up)
{
	SetLookDirection(at - eye, up);
	SetPosition(eye);
}

void IBaseCamera::SetLookDirection(Math::Vector3 forward, Math::Vector3 up)
{
	// Given, but ensure normalization
	Math::Scalar forwardLenSq = LengthSquare(forward);
	forward = Select(forward * RecipSqrt(forwardLenSq), -Math::Vector3(Math::EZUnitVector::kZUnitVector), forwardLenSq < Math::Scalar(0.000001f));

	// Deduce a valid, orthogonal right vector
	Math::Vector3 right = Cross(forward, up);
	Math::Scalar rightLenSq = LengthSquare(right);
	right = Select(right * RecipSqrt(rightLenSq), Math::Quaternion(Math::Vector3(Math::EYUnitVector::kYUnitVector), -DirectX::XM_PIDIV2) * forward, rightLenSq < Math::Scalar(0.000001f));

	// Compute actual up vector
	up = Cross(right, forward);

	// Finish constructing basis
	m_Basis = Math::Matrix3(right, up, -forward);
	m_CameraToWorld.SetRotation(Math::Quaternion(m_Basis)); // Initial value of reference to non-const must be an lvalue.
}

void IBaseCamera::SetRotation(Math::Quaternion basisRotation)
{
	m_CameraToWorld.SetRotation(Math::Quaternion(DirectX::XMQuaternionNormalize(basisRotation))); // ""
	m_Basis = Math::Matrix3(m_CameraToWorld.GetRotation());
}

void IBaseCamera::SetPosition(Math::Vector3 worldPos)
{
	m_CameraToWorld.SetTranslation(worldPos);
}

void IBaseCamera::SetTransform(const Math::AffineTransform& xform)
{
	SetLookDirection(-xform.GetZ(), xform.GetY());
	SetPosition(xform.GetTranslation());
}

void IBaseCamera::SetTransform(const Math::OrthogonalTransform& xform)
{
	SetRotation(xform.GetRotation());
	SetPosition(xform.GetTranslation());
}