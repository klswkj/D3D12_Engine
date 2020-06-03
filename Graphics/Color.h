#pragma once

#include <DirectXMath.h>

class Color
{
public:
    Color(float r, float g, float b, float a = 1.0f);
    Color(DirectX::FXMVECTOR vec);
    Color(const DirectX::XMVECTORF32& vec);
    Color(uint16_t r, uint16_t g, uint16_t b, uint16_t a = 255, uint16_t bitDepth = 8);
    explicit Color(uint32_t rgbaLittleEndian);
    Color() : m_value(DirectX::g_XMOne)
    {
    }

    float R() const 
    { 
        return DirectX::XMVectorGetX(m_value); 
    }

    float G() const 
    { 
        return DirectX::XMVectorGetY(m_value); 
    }

    float B() const 
    { 
        return DirectX::XMVectorGetZ(m_value); 
    }

    float A() const 
    { 
        return DirectX::XMVectorGetW(m_value); 
    }

    bool operator==(const Color& rhs) const 
    { 
        return DirectX::XMVector4Equal(m_value, rhs.m_value); 
    }
    bool operator!=(const Color& rhs) const 
    { 
        return !DirectX::XMVector4Equal(m_value, rhs.m_value); 
    }

    void SetR(float r) 
    { 
        m_value.f[0] = r; 
    }
    void SetG(float g) 
    { 
        m_value.f[1] = g; 
    }
    void SetB(float b) 
    { 
        m_value.f[2] = b; 
    }
    void SetA(float a) 
    { 
        m_value.f[3] = a; 
    }

    float* GetPtr(void) 
    { 
        return reinterpret_cast<float*>(this); 
    }
    float& operator[](int idx) 
    { 
        return GetPtr()[idx]; 
    }

    void SetRGB(float r, float g, float b) 
    { 
        m_value.v = DirectX::XMVectorSelect
        (
            m_value, 
            DirectX::XMVectorSet(r, g, b, b), DirectX::g_XMMask3
        ); 
    }

    Color ToSRGB() const;
    Color FromSRGB() const;
    Color ToREC709() const;
    Color FromREC709() const;

    // Probably want to convert to sRGB or Rec709 first
    uint32_t R10G10B10A2() const;
    uint32_t R8G8B8A8() const;

    // Pack an HDR color into 32-bits
    uint32_t R11G11B10F(bool RoundToEven = false) const;
    uint32_t R9G9B9E5() const;

    operator DirectX::XMVECTOR() const 
    { 
        return m_value; 
    }

private:
    DirectX::XMVECTORF32 m_value = {};
};