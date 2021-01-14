#pragma once

#include "MathCommon.h"

namespace Math
{
    // DirectX::XMVECTOR
    class Scalar
    {
    public:
        __forceinline Scalar() { m_vec = SplatZero(); }
        __forceinline Scalar(const Scalar& s) { m_vec = s; }
        __forceinline Scalar(float f) { m_vec = DirectX::XMVectorReplicate(f); }
        __forceinline explicit Scalar(DirectX::FXMVECTOR vec) { m_vec = vec; }
        __forceinline explicit Scalar(EZeroTag) { m_vec = SplatZero(); }
        __forceinline explicit Scalar(EIdentityTag) { m_vec = SplatOne(); }

        __forceinline operator DirectX::XMVECTOR() const { return m_vec; }
        __forceinline operator float() const { return DirectX::XMVectorGetX(m_vec); }

    private:
        DirectX::XMVECTOR m_vec;
    };

    __forceinline Scalar operator- (Scalar s) { return Scalar(DirectX::XMVectorNegate(s)); }
    __forceinline Scalar operator+ (Scalar s1, Scalar s2) { return Scalar(DirectX::XMVectorAdd(s1, s2)); }
    __forceinline Scalar operator- (Scalar s1, Scalar s2) { return Scalar(DirectX::XMVectorSubtract(s1, s2)); }
    __forceinline Scalar operator* (Scalar s1, Scalar s2) { return Scalar(DirectX::XMVectorMultiply(s1, s2)); }
    __forceinline Scalar operator/ (Scalar s1, Scalar s2) { return Scalar(DirectX::XMVectorDivide(s1, s2)); }
    __forceinline Scalar operator+ (Scalar s1, float s2) { return s1 + Scalar(s2); }
    __forceinline Scalar operator- (Scalar s1, float s2) { return s1 - Scalar(s2); }
    __forceinline Scalar operator* (Scalar s1, float s2) { return s1 * Scalar(s2); }
    __forceinline Scalar operator/ (Scalar s1, float s2) { return s1 / Scalar(s2); }
    __forceinline Scalar operator+ (float s1, Scalar s2) { return Scalar(s1) + s2; }
    __forceinline Scalar operator- (float s1, Scalar s2) { return Scalar(s1) - s2; }
    __forceinline Scalar operator* (float s1, Scalar s2) { return Scalar(s1) * s2; }
    __forceinline Scalar operator/ (float s1, Scalar s2) { return Scalar(s1) / s2; }

} // namespace Math