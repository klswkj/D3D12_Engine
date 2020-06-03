#include "Color.h"
#include <algorithm>

inline Color::Color(DirectX::FXMVECTOR vec)
{
    m_value.v = vec;
}

inline Color::Color(const DirectX::XMVECTORF32& vec)
{
    m_value = vec;
}

inline Color::Color(float r, float g, float b, float a)
{
    m_value.v = DirectX::XMVectorSet(r, g, b, a);
}

inline Color::Color(uint16_t r, uint16_t g, uint16_t b, uint16_t a, uint16_t bitDepth)
{
    m_value.v = DirectX::XMVectorScale(DirectX::XMVectorSet(r, g, b, a), 1.0f / ((1 << bitDepth) - 1));
}

inline Color::Color(uint32_t u32)
{
    float r = (float)((u32 >> 0) & 0xFF);
    float g = (float)((u32 >> 8) & 0xFF);
    float b = (float)((u32 >> 16) & 0xFF);
    float a = (float)((u32 >> 24) & 0xFF);
    m_value.v = DirectX::XMVectorScale(DirectX::XMVectorSet(r, g, b, a), 1.0f / 255.0f);
}

inline Color Color::ToSRGB(void) const
{
    DirectX::XMVECTOR T = XMVectorSaturate(m_value);
	DirectX::XMVECTOR result = DirectX::XMVectorSubtract
	(
		DirectX::XMVectorScale
		(
			DirectX::XMVectorPow(T, DirectX::XMVectorReplicate(1.0f / 2.4f)), 1.055f
        ),
        DirectX::XMVectorReplicate(0.055f)
	);
    result = DirectX::XMVectorSelect
    (
        result, DirectX::XMVectorScale(T, 12.92f), 
        DirectX::XMVectorLess(T, DirectX::XMVectorReplicate(0.0031308f))
    );
    return DirectX::XMVectorSelect(T, result, DirectX::g_XMSelect1110);
}

inline Color Color::FromSRGB(void) const
{
    DirectX::XMVECTOR T = XMVectorSaturate(m_value);
    DirectX::XMVECTOR result = DirectX::XMVectorPow(DirectX::XMVectorScale(DirectX::XMVectorAdd(T, DirectX::XMVectorReplicate(0.055f)), 1.0f / 1.055f), DirectX::XMVectorReplicate(2.4f));
    result = DirectX::XMVectorSelect(result, DirectX::XMVectorScale(T, 1.0f / 12.92f), DirectX::XMVectorLess(T, DirectX::XMVectorReplicate(0.0031308f)));
    return DirectX::XMVectorSelect(T, result, DirectX::g_XMSelect1110);
}

inline Color Color::ToREC709(void) const
{
    DirectX::XMVECTOR T = DirectX::XMVectorSaturate(m_value);
    DirectX::XMVECTOR result = DirectX::XMVectorSubtract(DirectX::XMVectorScale(DirectX::XMVectorPow(T, DirectX::XMVectorReplicate(0.45f)), 1.099f), DirectX::XMVectorReplicate(0.099f));
    result = DirectX::XMVectorSelect(result, DirectX::XMVectorScale(T, 4.5f), DirectX::XMVectorLess(T, DirectX::XMVectorReplicate(0.0018f)));
    return DirectX::XMVectorSelect(T, result, DirectX::g_XMSelect1110);
}

inline Color Color::FromREC709(void) const
{
    DirectX::XMVECTOR T = XMVectorSaturate(m_value);
    DirectX::XMVECTOR result = DirectX::XMVectorPow(DirectX::XMVectorScale(DirectX::XMVectorAdd(T, DirectX::XMVectorReplicate(0.099f)), 1.0f / 1.099f), DirectX::XMVectorReplicate(1.0f / 0.45f));
    result = DirectX::XMVectorSelect(result, DirectX::XMVectorScale(T, 1.0f / 4.5f), DirectX::XMVectorLess(T, DirectX::XMVectorReplicate(0.0081f)));
    return DirectX::XMVectorSelect(T, result, DirectX::g_XMSelect1110);
}

inline uint32_t Color::R10G10B10A2(void) const
{
    DirectX::XMVECTOR result = DirectX::XMVectorRound(DirectX::XMVectorMultiply(XMVectorSaturate(m_value), DirectX::XMVectorSet(1023.0f, 1023.0f, 1023.0f, 3.0f)));
    result = _mm_castsi128_ps(_mm_cvttps_epi32(result));
    uint32_t r = DirectX::XMVectorGetIntX(result);
    uint32_t g = DirectX::XMVectorGetIntY(result);
    uint32_t b = DirectX::XMVectorGetIntZ(result);
    uint32_t a = DirectX::XMVectorGetIntW(result) >> 8;
    return a << 30 | b << 20 | g << 10 | r;
}

inline uint32_t Color::R8G8B8A8(void) const
{
    DirectX::XMVECTOR result = DirectX::XMVectorRound(DirectX::XMVectorMultiply(XMVectorSaturate(m_value), DirectX::XMVectorReplicate(255.0f)));
    result = _mm_castsi128_ps(_mm_cvttps_epi32(result));
    uint32_t r = DirectX::XMVectorGetIntX(result);
    uint32_t g = DirectX::XMVectorGetIntY(result);
    uint32_t b = DirectX::XMVectorGetIntZ(result);
    uint32_t a = DirectX::XMVectorGetIntW(result);
    return a << 24 | b << 16 | g << 8 | r;
}

uint32_t Color::R11G11B10F(bool RoundToEven) const
{
#if defined(_XM_NO_INTRINSICS_) | !defined(_XM_SSE_INTRINSICS_)
    static const float kMaxVal = float(1 << 16);
    static const float kF32toF16 = (1.0 / (1ull << 56)) * (1.0 / (1ull << 56));

    union { float f; uint32_t u; } R, G, B;

    R.f = std::clamp(m_value.f[0], 0.0f, kMaxVal) * kF32toF16;
    G.f = std::clamp(m_value.f[1], 0.0f, kMaxVal) * kF32toF16;
    B.f = std::clamp(m_value.f[2], 0.0f, kMaxVal) * kF32toF16;

    if (RoundToEven)
    {
        // Bankers rounding:  2.5 -> 2.0  ;  3.5 -> 4.0
        R.u += 0x0FFFF + ((R.u >> 16) & 1);
        G.u += 0x0FFFF + ((G.u >> 16) & 1);
        B.u += 0x1FFFF + ((B.u >> 17) & 1);
    }
    else
    {
        // Default rounding:  2.5 -> 3.0  ;  3.5 -> 4.0
        R.u += 0x00010000;
        G.u += 0x00010000;
        B.u += 0x00020000;
    }

    R.u &= 0x0FFE0000;
    G.u &= 0x0FFE0000;
    B.u &= 0x0FFC0000;

    return R.u >> 17 | G.u >> 6 | B.u << 4;

#elif !defined(_XM_NO_INTRINSICS_) | defined(_XM_SSE_INTRINSICS_)

    static DirectX::XMVECTORU32 Scale = { 0x07800000, 0x07800000, 0x07800000, 0 }; // 2^-112
    static DirectX::XMVECTORU32 Round1 = { 0x00010000, 0x00010000, 0x00020000, 0 };
    static DirectX::XMVECTORU32 Round2 = { 0x0000FFFF, 0x0000FFFF, 0x0001FFFF, 0 };
    static DirectX::XMVECTORU32 Mask = { 0x0FFE0000, 0x0FFE0000, 0x0FFC0000, 0 };

    // Treat the values like integers as we clamp to [0, +Inf].  
    // This translates 32-bit specials to 16-bit specials.
    // (while also turning anything greater than MAX_HALF into +INF)
    __m128i ti = _mm_max_epi32(_mm_castps_si128(m_value), _mm_setzero_si128());
    ti = _mm_min_epi32(ti, _mm_set1_epi32(0x47800000)); // 2^16 = 65536.0f = INF

    // Bias the exponent by -112 (-127 + 15) to denormalize values < 2^-14
    ti = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(ti), Scale));

    if (RoundToEven)
    {
        // Add 0x10000 when odd, 0x0FFFF when even (before truncating bits)
        ti = _mm_add_epi32(ti, _mm_max_epi32(_mm_and_si128(_mm_srli_epi32(ti, 1), Round1), Round2));
    }
    else //if (RoundToNearest)
    {
        ti = _mm_add_epi32(ti, Round1);
    }

    DirectX::XMVECTORU32 ret;
    ret.v = _mm_castsi128_ps(_mm_and_si128(ti, Mask));
    return ret.u[0] >> 17 | ret.u[1] >> 6 | ret.u[2] << 4;

#endif
}

uint32_t Color::R9G9B9E5() const
{
#if defined(_XM_NO_INTRINSICS_) | !defined(_XM_SSE_INTRINSICS_)
    static const float kMaxVal = float(0x1FF << 7);
    static const float kMinVal = float(1.f / (1 << 16));

    // Clamp RGB to [0, 1.FF*2^16]
    float r = std::clamp(m_value.f[0], 0.0f, kMaxVal);
    float g = std::clamp(m_value.f[1], 0.0f, kMaxVal);
    float b = std::clamp(m_value.f[2], 0.0f, kMaxVal);

    // Compute the maximum channel, no less than 1.0*2^-15
    float MaxChannel = std::max(std::max(r, g), std::max(b, kMinVal));

    // Take the exponent of the maximum channel (rounding up the 9th bit) and
    // add 15 to it.  
    // When added to the channels, it causes the implicit '1.0' bit and
    // the first 8 mantissa bits to be shifted down to the low 9 bits of the mantissa,
    // rounding the truncated bits.
    union { float f; int32_t i; } R, G, B, E;
    E.f = MaxChannel;
    E.i += 0x07804000; // Add 15 to the exponent and 0x4000 to the mantissa
    E.i &= 0x7F800000; // Zero the mantissa

    // This shifts the 9-bit values we need into the lowest bits, rounding as
    // needed.  Note that if the channel has a smaller exponent than the max
    // channel, it will shift even more.  This is intentional.
    R.f = r + E.f;
    G.f = g + E.f;
    B.f = b + E.f;

    // Convert the Bias to the correct exponent in the upper 5 bits.
    E.i <<= 4;
    E.i += 0x10000000;

    // Combine the fields.  RGB floats have unwanted data in the upper 9
    // bits.  Only red needs to mask them off because green and blue shift
    // it out to the left.
    return E.i | B.i << 18 | G.i << 9 | R.i & 511;

#elif !defined(_XM_NO_INTRINSICS_) | defined(_XM_SSE_INTRINSICS_)

    // Clamp RGB to [0, 1.FF*2^16]
    __m128 kMaxVal = _mm_castsi128_ps(_mm_set1_epi32(0x477F8000));
    __m128 rgb = _mm_min_ps(_mm_max_ps(m_value, _mm_setzero_ps()), kMaxVal);

    // Compute the maximum channel, no less than 1.0*2^-15
    __m128 kMinVal = _mm_castsi128_ps(_mm_set1_epi32(0x37800000));
    __m128 MaxChannel = _mm_max_ps(rgb, kMinVal);
    MaxChannel = _mm_max_ps(_mm_permute_ps(MaxChannel, _MM_SHUFFLE(3, 1, 0, 2)),
        _mm_max_ps(_mm_permute_ps(MaxChannel, _MM_SHUFFLE(3, 0, 2, 1)), MaxChannel));

    // Add 15 to the exponent and 0x4000 to the mantissa
    __m128i kBias15 = _mm_set1_epi32(0x07804000);
    __m128i kExpMask = _mm_set1_epi32(0x7F800000);
    __m128i Bias = _mm_and_si128(_mm_add_epi32(_mm_castps_si128(MaxChannel), kBias15), kExpMask);

    // rgb += Bias
    rgb = _mm_add_ps(rgb, _mm_castsi128_ps(Bias));

    // Exp = (Bias << 4) + 0x10000000;
    __m128i Exp = _mm_add_epi32(_mm_slli_epi32(Bias, 4), _mm_set1_epi32(0x10000000));

    // Combine words
    DirectX::XMVECTORU32 ret;
    ret.v = _mm_insert_ps(rgb, _mm_castsi128_ps(Exp), 0x30);
    return ret.u[3] | ret.u[2] << 18 | ret.u[1] << 9 | ret.u[0] & 511;

#endif
}