#include "stdafx.h"

/*
template<typename T>
inline void SafeRelease(T*& rpInterface)
{
    T* pInterface = rpInterface;

    if (pInterface)
    {
        pInterface->Release();
        rpInterface = nullptr;
    }
}

template<typename T>
inline void SafeDelete(T*& rpObject)
{
    T* pObject = rpObject;

    if (pObject)
    {
        delete pObject;
        rpObject = nullptr;
    }
}
*/

// A faster version of memcopy that uses SSE instructions.  
// TODO:  Write an ARM variant if necessary.
void SIMDMemCopy(void* __restrict _Dest, const void* __restrict _Source, size_t NumQuadwords)
{
    ASSERT(HashInternal::IsAligned(_Dest, 16));
    ASSERT(HashInternal::IsAligned(_Source, 16));

    __m128i* __restrict Dest = (__m128i * __restrict)_Dest;
    const __m128i* __restrict Source = (const __m128i * __restrict)_Source;

    // Discover how many quadwords precede a cache line boundary.  
    // Copy them separately.
    size_t InitialQuadwordCount = (4 - ((size_t)Source >> 4) & 3) & 3;
    if (NumQuadwords < InitialQuadwordCount)
    {
        InitialQuadwordCount = NumQuadwords;
    }

    switch (InitialQuadwordCount)
    {
        case 3: _mm_stream_si128(Dest + 2, _mm_load_si128(Source + 2));     // Fall through
        case 2: _mm_stream_si128(Dest + 1, _mm_load_si128(Source + 1));     // Fall through
        case 1: _mm_stream_si128(Dest + 0, _mm_load_si128(Source + 0));     // Fall through
        default:
            break;
    }

    if (NumQuadwords == InitialQuadwordCount)
    {
        return;
    }

    Dest += InitialQuadwordCount;
    Source += InitialQuadwordCount;
    NumQuadwords -= InitialQuadwordCount;

    size_t CacheLines = NumQuadwords >> 2;

    switch (CacheLines)
    {
    default:
    case 10: _mm_prefetch((char*)(Source + 36), _MM_HINT_NTA);    // Fall through
    case 9:  _mm_prefetch((char*)(Source + 32), _MM_HINT_NTA);    // Fall through
    case 8:  _mm_prefetch((char*)(Source + 28), _MM_HINT_NTA);    // Fall through
    case 7:  _mm_prefetch((char*)(Source + 24), _MM_HINT_NTA);    // Fall through
    case 6:  _mm_prefetch((char*)(Source + 20), _MM_HINT_NTA);    // Fall through
    case 5:  _mm_prefetch((char*)(Source + 16), _MM_HINT_NTA);    // Fall through
    case 4:  _mm_prefetch((char*)(Source + 12), _MM_HINT_NTA);    // Fall through
    case 3:  _mm_prefetch((char*)(Source + 8), _MM_HINT_NTA);    // Fall through
    case 2:  _mm_prefetch((char*)(Source + 4), _MM_HINT_NTA);    // Fall through
    case 1:  _mm_prefetch((char*)(Source + 0), _MM_HINT_NTA);    // Fall through

        // Do four quadwords per loop to minimize stalls.
        for (size_t i{ CacheLines }; 0 < i; --i)
        {
            // If this is a large copy, start prefetching future cache lines.  This also prefetches the
            // trailing quadwords that are not part of a whole cache line.
            if (10 <= i)
            {
                _mm_prefetch((char*)(Source + 40), _MM_HINT_NTA);
            }
            _mm_stream_si128(Dest + 0, _mm_load_si128(Source + 0));
            _mm_stream_si128(Dest + 1, _mm_load_si128(Source + 1));
            _mm_stream_si128(Dest + 2, _mm_load_si128(Source + 2));
            _mm_stream_si128(Dest + 3, _mm_load_si128(Source + 3));

            Dest += 4;
            Source += 4;
        }
    }

    // Copy the remaining quadwords
    switch (NumQuadwords & 3)
    {
    case 3: _mm_stream_si128(Dest + 2, _mm_load_si128(Source + 2));     // Fall through
    case 2: _mm_stream_si128(Dest + 1, _mm_load_si128(Source + 1));     // Fall through
    case 1: _mm_stream_si128(Dest + 0, _mm_load_si128(Source + 0));     // Fall through
    default:
        break;
    }

    _mm_sfence();
}

void SIMDMemFill(void* __restrict _Dest, __m128 FillVector, size_t NumQuadwords)
{
    ASSERT(HashInternal::IsAligned(_Dest, 16));
    // Keyword 'Register' is no longer a supported storage class
    /*register */const __m128i Source = _mm_castps_si128(FillVector);
    __m128i* __restrict Dest = (__m128i * __restrict)_Dest;

    if (((size_t)Dest >> 4) & 3)
    {
        _mm_stream_si128(Dest++, Source);
        --NumQuadwords;
        // Fall through
    }

    size_t WholeCacheLines = NumQuadwords >> 2;

    // Do four quadwords per loop to minimize stalls.
    while (WholeCacheLines--)
    {
        _mm_stream_si128(Dest++, Source);
        _mm_stream_si128(Dest++, Source);
        _mm_stream_si128(Dest++, Source);
        _mm_stream_si128(Dest++, Source);
    }

    // Copy the remaining quadwords
    if (((size_t)Dest >> 4) & 3)
    {
        _mm_stream_si128(Dest++, Source);
        --NumQuadwords;
        // Fall through
    }

    _mm_sfence();
}


template<class Iter>
void SplitStringIter(const std::string& s, const char* delim, Iter out)
{
    if (delim == nullptr)
    {
        *out++ = s;
    }
    else
    {
        size_t a = 0;
        size_t b = s.find(delim);

        for (; b != std::string::npos;
            a = b + strlen(delim), b = s.find(delim, a))
        {
            *out++ = std::move(s.substr(a, b - a));
        }
        *out++ = std::move(s.substr(a, s.length() - a));
    }
}

std::vector<std::string> SplitString(const std::string& s, const char* delim)
{
    std::vector<std::string> strings;
    SplitStringIter(s, delim, std::back_inserter(strings));
    return strings;
}

std::wstring StringToWString(const std::string& str)
{
    wchar_t buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

std::wstring AnsiToWString(const char* str)
{
    wchar_t buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str, -1, buffer, 512);
    return std::wstring(buffer);
}

std::string _NormalizeString(const std::string& InputString)
{
    size_t FirstDotIndex = -1;       // std::string::npos
    size_t FirstBackSlashIndex = -1; // std::string::npos 

    FirstDotIndex = InputString.rfind('.');        // strrchr(InputString.c_str(), '.');
    FirstBackSlashIndex = InputString.rfind('\\'); // strrchr(InputString.c_str(), '\\');

    ASSERT(FirstDotIndex != std::string::npos && FirstBackSlashIndex != std::string::npos);

    return InputString.substr(FirstBackSlashIndex + 1, FirstDotIndex - FirstBackSlashIndex - 1);
}

std::string SplitFormat(const std::string InputString)
{
    size_t FirstDotIndex = -1;

    FirstDotIndex = InputString.rfind('.'); // // strrchr(InputString.c_str(), '.');

    ASSERT(FirstDotIndex != std::string::npos);

    return InputString.substr(0, FirstDotIndex);
}

std::wstring _NormalizeWString(const std::wstring& InputString)
{
    // size_t FirstDotIndex = -1;       // std::string::npos
    size_t FirstBackSlashIndex = -1; // std::string::npos 

    // FirstDotIndex = InputString.rfind('.');        // strrchr(InputString.c_str(), '.');
    FirstBackSlashIndex = InputString.rfind('\\'); // strrchr(InputString.c_str(), '\\');

    // ASSERT(FirstDotIndex != std::string::npos && FirstBackSlashIndex != std::wstring::npos);

    // return InputString.substr(FirstBackSlashIndex + 1, FirstDotIndex - FirstBackSlashIndex - 1);
    return InputString.substr(FirstBackSlashIndex + 1);
}
std::wstring SplitFormat(const std::wstring InputString)
{
    size_t FirstDotIndex = -1;

    FirstDotIndex = InputString.rfind('.'); // // strrchr(InputString.c_str(), '.');

    ASSERT(FirstDotIndex != std::wstring::npos);

    return InputString.substr(0, FirstDotIndex);
}


void SetName(ID3D12Object* d3dObject, std::string FileName, std::string FunctionName, size_t LineNumber)
{
#ifndef RELEASE
    std::wstring name;

    size_t firstchar = 0;

    for (size_t i{ FileName.size() }; 0 < i; --i)
    {
        if (FileName[i] == '\\')
        {
            firstchar = i;
            break;
        }
    }

    name += StringToWString(FileName.substr(firstchar + 1));
    name += L", Function : ";
    name += StringToWString(FunctionName);
    name += L", Line : ";
    name += std::to_wstring(LineNumber);

    d3dObject->SetName(name.c_str());
#endif
}

DirectX::XMFLOAT3 ExtractEulerAngles(const DirectX::XMFLOAT4X4& Matrix)
{
    DirectX::XMFLOAT3 euler;

    euler.x = asinf(-Matrix._32);                  // Pitch
    if (cosf(euler.x) > 0.0001)                    // If Not at poles
    {
        euler.y = atan2f(Matrix._31, Matrix._33);  // Yaw
        euler.z = atan2f(Matrix._12, Matrix._22);  // Roll
    }
    else
    {
        euler.y = 0.0f;                            // Yaw
        euler.z = atan2f(-Matrix._21, Matrix._11); // Roll
    }

    return euler;
}

DirectX::XMFLOAT3 ExtractTranslation(const DirectX::XMFLOAT4X4& Matrix)
{
    return { Matrix._41, Matrix._42, Matrix._43 };
}

DirectX::XMMATRIX ScaleTranslation(DirectX::XMMATRIX Matrix, float Scale)
{
    Matrix.r[3].m128_f32[0] *= Scale;
    Matrix.r[3].m128_f32[1] *= Scale;
    Matrix.r[3].m128_f32[2] *= Scale;
    return Matrix;
}
