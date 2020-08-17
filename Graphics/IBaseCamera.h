#pragma once

#include "MathBasic.h"
#include "MathCommon.h"
#include "Frustum.h"

#define ShaderModelToProjection

//   Quat      <->    Directional Vector   <-> Euler Angle
// Quaternion < -> { Right, Up, Forward } < -> { Roll, Pitch, Yaw }


// 1. Quaternion <-> RightUpForward(Matrix or vector)
//    1 - 1. Quaternion->Directional Vector
//    RightUpForward = DirectX::XMMatrixRotationQuaternion(q)->return 

//    1 - 2. Directional Vector->Quaternion
//    DirectX::XMQuaternionRotationMatrix(matrix)


//    2. Quaternion <-> Euler Angle(RollPitchYaw)
//    2 - 1. Quaternion->Euler Angle
//    XMQuaternionToAxisAngle( XMVECTOR *pAxis, float* pAngle, FXMVECTOR Q) -> Testing Failed.
//    Gimbal Lock.
//    2 - 2. Euler Angle -> Quaternion
//    returnQuaternion = XMQuaternionRotationRollPitchYawFromVector(FXMVECTOR::Angles[pitch, yaw, roll, don't care])
//    returnQuaternion = DirectX::XMQuaternionRotationRollPitchYaw(float pitch, float yaw, float roll)

//    3. Directional Vector <-> Euler Angle
//    3 - 1. Directonal Vector->Euler Angle
//    (1-2), (2-1)
//    3 - 2. Euler Angle->Directional Vector
//    EulerAngle convert to Quaternion(2-2), then convert Directional Vector(1-1).

// Quaternion <-> Euler Angle 이거는 안하는걸로

// 1. 모든 인풋은 마우스 Y축-> yaw로, 마우스 X축 -> Pitch
// 2. yaw   -> MakeYRotation(기존 + 새로운 인풋),
//    Pitch -> makeXRotation(기존 + 새로운 인풋)
// => Matrix3::Orientation = Matrix3(World Right, World Up, World Forward)  * MakeYRot * MakeXRot
// => Position = 

class IBaseCamera
{
public:

    // Call this function once per frame and after you've changed any state.  This
    // regenerates all matrices.  Calling it more or less than once per frame will break
    // temporal effects and cause unpredictable results.
    void Update();

    // Public functions for controlling where the camera is and its orientation
    // Set Matrix3 m_Basis, OrthogonalTransform m_CameraToWorld.
    void SetEyeAtUp(Math::Vector3 eye, Math::Vector3 at, Math::Vector3 up);

    void SetLookDirection(Math::Vector3 forward, Math::Vector3 up);
    void SetRotation(Math::Quaternion basisRotation);
    void SetPosition(Math::Vector3 worldPos);
    void SetTransform(const Math::AffineTransform& xform);
    void SetTransform(const Math::OrthogonalTransform& xform);

    const Math::Quaternion GetRotation() const { return m_CameraToWorld.GetRotation(); }
    const Math::Matrix3 GetRightUpForwardMatrix() const { return m_Basis; }
    const Math::Vector3 GetRightVec() const { return m_Basis.GetX(); }
    const Math::Vector3 GetUpVec() const { return m_Basis.GetY(); }
    const Math::Vector3 GetForwardVec() const { return -m_Basis.GetZ(); }
    const Math::Vector3 GetPosition() const { return m_CameraToWorld.GetTranslation(); }

    const Math::Matrix4& GetProjMatrix() const { return m_ProjMatrix; }
    const Math::Matrix4& GetViewProjMatrix() const { return m_ViewProjMatrix; }
    const Math::Matrix4& GetReprojectionMatrix() const { return m_ReprojectMatrix; }
    const Math::Frustum& GetViewSpaceFrustum() const { return m_FrustumVS; }
    const Math::Frustum& GetWorldSpaceFrustum() const { return m_FrustumWS; }

protected:
    IBaseCamera() : m_CameraToWorld(Math::EIdentityTag::kIdentity), m_Basis(Math::EIdentityTag::kIdentity) {}

    void SetProjMatrix(const Math::Matrix4& ProjMat) { m_ProjMatrix = ProjMat; }

    // Redundant data cached for faster lookups.
    // [0] -> right, [1] -> up, [2] -> -forward
    Math::Matrix3 m_Basis;

    Math::OrthogonalTransform m_CameraToWorld; // ViewMatrix

    // Transforms homogeneous coordinates from world space to view space.  In this case, view space is defined as +X is
    // to the right, +Y is up, and -Z is forward.  This has to match what the projection matrix expects, but you might
    // also need to know what the convention is if you work in view space in a shader.
    // m_ViewMatrix = Matrix4(~m_CameraToWorld);
    // -> m_Viewmatrix = Matrix4(XMMatrixRotationQuaternion(~m_CameraToWorld.Quaternion), (~m_CameraToWorld.Quaternion * m_CameraToworld.Translation))
    // Math::Matrix4 m_ViewMatrix;        // i.e. "World-to-View" matrix

    // The projection matrix transforms view space to clip space.  Once division by W has occurred, the final coordinates
    // can be transformed by the viewport matrix to screen space.  The projection matrix is determined by the screen aspect 
    // and camera field of view.  A projection matrix can also be orthographic.  In that case, field of view would be defined
    // in linear units, not angles.
    Math::Matrix4 m_ProjMatrix;             // i.e. "View-to-Projection" matrix

    // // A concatenation of the view and projection matrices.
    ShaderModelToProjection Math::Matrix4 m_ViewProjMatrix; // m_ProjMatrix * m_ViewMatrix ->      
    Math::Matrix4 m_PreviousViewProjMatrix; // The view-projection matrix from the previous frame
    Math::Matrix4 m_ReprojectMatrix;        // Projects a clip-space coordinate to the previous frame (useful for TAA).

    Math::Frustum m_FrustumVS;        // View-space view frustum
    Math::Frustum m_FrustumWS;        // World-space view frustum
};
