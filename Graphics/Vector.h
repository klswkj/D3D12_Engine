#pragma once

#include "Scalar.h"
#include "MathCommon.h"

class Camera;

namespace Math
{
    class Vector4;

    // A 3-vector with an unspecified fourth component.  Depending on the context, the W can be 0 or 1, but both are implicit.
    // The actual value of the fourth component is undefined for performance reasons.
    class Vector3
    {
        friend Camera;
    public:
        __forceinline Vector3() { m_vec = SplatOne(); }
        __forceinline Vector3(float x, float y, float z) { m_vec = DirectX::XMVectorSet(x, y, z, z); }
        __forceinline Vector3(const DirectX::XMFLOAT3& v) { m_vec = DirectX::XMLoadFloat3(&v); }
        __forceinline Vector3(const Vector3& v) { m_vec = v; }
        __forceinline Vector3(Scalar s) { m_vec = s; }
        __forceinline explicit Vector3(Vector4 v);
        __forceinline explicit Vector3(DirectX::FXMVECTOR vec) { m_vec = vec; }
        __forceinline explicit Vector3(EZeroTag) { m_vec = SplatZero(); }
        __forceinline explicit Vector3(EIdentityTag) { m_vec = SplatOne(); }
        __forceinline explicit Vector3(EXUnitVector) { m_vec = CreateXUnitVector(); }
        __forceinline explicit Vector3(EYUnitVector) { m_vec = CreateYUnitVector(); }
        __forceinline explicit Vector3(EZUnitVector) { m_vec = CreateZUnitVector(); }

        __forceinline operator DirectX::XMVECTOR() const { return m_vec; }
        __forceinline operator DirectX::XMFLOAT3() const { return DirectX::XMFLOAT3(m_vec.m128_f32[0], m_vec.m128_f32[1], m_vec.m128_f32[2]); }
        __forceinline operator DirectX::XMFLOAT4() const { return DirectX::XMFLOAT4(m_vec.m128_f32[0], m_vec.m128_f32[1], m_vec.m128_f32[2], 0.0f); }

        __forceinline Scalar GetX() const { return Scalar(DirectX::XMVectorSplatX(m_vec)); }
        __forceinline Scalar GetY() const { return Scalar(DirectX::XMVectorSplatY(m_vec)); }
        __forceinline Scalar GetZ() const { return Scalar(DirectX::XMVectorSplatZ(m_vec)); }
        __forceinline void SetX(Scalar x) { m_vec = DirectX::XMVectorPermute<4, 1, 2, 3>(m_vec, x); }
        __forceinline void SetY(Scalar y) { m_vec = DirectX::XMVectorPermute<0, 5, 2, 3>(m_vec, y); }
        __forceinline void SetZ(Scalar z) { m_vec = DirectX::XMVectorPermute<0, 1, 6, 3>(m_vec, z); }

        __forceinline Vector3 operator- () const { return Vector3(DirectX::XMVectorNegate(m_vec)); }
        __forceinline Vector3 operator+ (Vector3 v2) const { return Vector3(DirectX::XMVectorAdd(m_vec, v2)); }
        __forceinline Vector3 operator- (Vector3 v2) const { return Vector3(DirectX::XMVectorSubtract(m_vec, v2)); }
        __forceinline Vector3 operator* (Vector3 v2) const { return Vector3(DirectX::XMVectorMultiply(m_vec, v2)); }
        __forceinline Vector3 operator/ (Vector3 v2) const { return Vector3(DirectX::XMVectorDivide(m_vec, v2)); }
        __forceinline Vector3 operator* (Scalar  v2) const { return *this * Vector3(v2); }
        __forceinline Vector3 operator/ (Scalar  v2) const { return *this / Vector3(v2); }
        __forceinline Vector3 operator* (float  v2) const { return *this * Scalar(v2); }
        __forceinline Vector3 operator/ (float  v2) const { return *this / Scalar(v2); }

        __forceinline Vector3& operator += (Vector3 v) { *this = *this + v; return *this; }
        __forceinline Vector3& operator -= (Vector3 v) { *this = *this - v; return *this; }
        __forceinline Vector3& operator *= (Vector3 v) { *this = *this * v; return *this; }
        __forceinline Vector3& operator /= (Vector3 v) { *this = *this / v; return *this; }

        __forceinline friend Vector3 operator* (Scalar  v1, Vector3 v2) { return Vector3(v1) * v2; }
        __forceinline friend Vector3 operator/ (Scalar  v1, Vector3 v2) { return Vector3(v1) / v2; }
        __forceinline friend Vector3 operator* (float   v1, Vector3 v2) { return Scalar(v1) * v2; }
        __forceinline friend Vector3 operator/ (float   v1, Vector3 v2) { return Scalar(v1) / v2; }

    protected:
        DirectX::XMVECTOR m_vec;
    }; // Vector3

    // A 4-vector, completely defined.
    class Vector4
    {
    public:
        __forceinline Vector4() {}
        __forceinline Vector4(float x, float y, float z, float w) { m_vec = DirectX::XMVectorSet(x, y, z, w); }
        __forceinline Vector4(Vector3 xyz, float w) { m_vec = DirectX::XMVectorSetW(xyz, w); }
        __forceinline Vector4(const Vector4& v) { m_vec = v; }
        __forceinline Vector4(const Scalar& s) { m_vec = s; }
        __forceinline explicit Vector4(Vector3 xyz) { m_vec = SetWToOne(xyz); }
        __forceinline explicit Vector4(DirectX::FXMVECTOR vec) { m_vec = vec; }
        __forceinline explicit Vector4(EZeroTag) { m_vec = SplatZero(); }
        __forceinline explicit Vector4(EIdentityTag) { m_vec = SplatOne(); }
        __forceinline explicit Vector4(EXUnitVector) { m_vec = CreateXUnitVector(); }
        __forceinline explicit Vector4(EYUnitVector) { m_vec = CreateYUnitVector(); }
        __forceinline explicit Vector4(EZUnitVector) { m_vec = CreateZUnitVector(); }
        __forceinline explicit Vector4(EWUnitVector) { m_vec = CreateWUnitVector(); }

        __forceinline operator DirectX::XMVECTOR() const { return m_vec; }
        __forceinline operator DirectX::XMFLOAT3() const { return DirectX::XMFLOAT3(m_vec.m128_f32[0], m_vec.m128_f32[1], m_vec.m128_f32[2]); }
        __forceinline operator DirectX::XMFLOAT4() const { return DirectX::XMFLOAT4(m_vec.m128_f32[0], m_vec.m128_f32[1], m_vec.m128_f32[2], m_vec.m128_f32[3]); }

        __forceinline Scalar GetX() const { return Scalar(DirectX::XMVectorSplatX(m_vec)); }
        __forceinline Scalar GetY() const { return Scalar(DirectX::XMVectorSplatY(m_vec)); }
        __forceinline Scalar GetZ() const { return Scalar(DirectX::XMVectorSplatZ(m_vec)); }
        __forceinline Scalar GetW() const { return Scalar(DirectX::XMVectorSplatW(m_vec)); }
        __forceinline void SetX(Scalar x) { m_vec = DirectX::XMVectorPermute<4, 1, 2, 3>(m_vec, x); }
        __forceinline void SetY(Scalar y) { m_vec = DirectX::XMVectorPermute<0, 5, 2, 3>(m_vec, y); }
        __forceinline void SetZ(Scalar z) { m_vec = DirectX::XMVectorPermute<0, 1, 6, 3>(m_vec, z); }
        __forceinline void SetW(Scalar w) { m_vec = DirectX::XMVectorPermute<0, 1, 2, 7>(m_vec, w); }

        __forceinline Vector4 operator- () const { return Vector4(DirectX::XMVectorNegate(m_vec)); }
        __forceinline Vector4 operator+ (Vector4 v2) const { return Vector4(DirectX::XMVectorAdd(m_vec, v2)); }
        __forceinline Vector4 operator- (Vector4 v2) const { return Vector4(DirectX::XMVectorSubtract(m_vec, v2)); }
        __forceinline Vector4 operator* (Vector4 v2) const { return Vector4(DirectX::XMVectorMultiply(m_vec, v2)); }
        __forceinline Vector4 operator/ (Vector4 v2) const { return Vector4(DirectX::XMVectorDivide(m_vec, v2)); }
        __forceinline Vector4 operator* (Scalar  v2) const { return *this * Vector4(v2); }
        __forceinline Vector4 operator/ (Scalar  v2) const { return *this / Vector4(v2); }
        __forceinline Vector4 operator* (float   v2) const { return *this * Scalar(v2); }
        __forceinline Vector4 operator/ (float   v2) const { return *this / Scalar(v2); }

        __forceinline void operator*= (float   v2) { *this = *this * Scalar(v2); }
        __forceinline void operator/= (float   v2) { *this = *this / Scalar(v2); }

        __forceinline friend Vector4 operator* (Scalar  v1, Vector4 v2) { return Vector4(v1) * v2; }
        __forceinline friend Vector4 operator/ (Scalar  v1, Vector4 v2) { return Vector4(v1) / v2; }
        __forceinline friend Vector4 operator* (float   v1, Vector4 v2) { return Scalar(v1) * v2; }
        __forceinline friend Vector4 operator/ (float   v1, Vector4 v2) { return Scalar(v1) / v2; }

    protected:
        DirectX::XMVECTOR m_vec{};
    }; // Vector4

    __forceinline Vector3::Vector3(Vector4 v)
    {
        Scalar W = v.GetW();
        m_vec = DirectX::XMVectorSelect(DirectX::XMVectorDivide(v, W), v, DirectX::XMVectorEqual(W, SplatZero()));
    }

    class BoolVector
    {
    public:
        __forceinline BoolVector(DirectX::FXMVECTOR vec) { m_vec = vec; }
        __forceinline operator DirectX::XMVECTOR() const { return m_vec; }
    protected:
        DirectX::XMVECTOR m_vec;
    };
} // namespace Math