#pragma once

#include "Vector.h"

#ifndef WARNING
#define WARNING
#endif

namespace Math
{
    class Quaternion
    {
    public:
        __forceinline Quaternion() { m_vec = DirectX::XMQuaternionIdentity(); }
        __forceinline Quaternion(const Vector3& axis, const Scalar& angle) { m_vec = DirectX::XMQuaternionRotationAxis(axis, angle); }
        __forceinline Quaternion(float pitch, float yaw, float roll) { m_vec = DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll); }
        
        // (RightVector, UpVector, -ForwardVector) to Quaternion.
        __forceinline explicit Quaternion(const DirectX::XMMATRIX& matrix) { m_vec = DirectX::XMQuaternionRotationMatrix(matrix); } 
        // WARNING __forceinline explicit Quaternion(const Matrix3& matrix) { m_vec = DirectX::XMQuaternionRotationMatrix(DirectX::XMMATRIX(matrix)); }
        __forceinline explicit Quaternion(const DirectX::FXMVECTOR vec) { m_vec = vec; }
        __forceinline explicit Quaternion(EIdentityTag) { m_vec = DirectX::XMQuaternionIdentity(); }

        __forceinline operator DirectX::XMVECTOR() const { return m_vec; }
        __forceinline Quaternion operator~ (void) const { return Quaternion(DirectX::XMQuaternionConjugate(m_vec)); }
        __forceinline Quaternion operator- (void) const { return Quaternion(DirectX::XMVectorNegate(m_vec)); }

        __forceinline Quaternion operator* (Quaternion rhs) const { return Quaternion(DirectX::XMQuaternionMultiply(rhs, m_vec)); }
        __forceinline Vector3 operator* (Vector3 rhs) const { return Vector3(DirectX::XMVector3Rotate(rhs, m_vec)); } // rhs is one of right up forward.

        __forceinline Quaternion& operator= (Quaternion rhs) { m_vec = rhs; return *this; }
        __forceinline Quaternion& operator*= (Quaternion rhs) { *this = *this * rhs; return *this; }

    protected:
        DirectX::XMVECTOR m_vec;
    };
}
