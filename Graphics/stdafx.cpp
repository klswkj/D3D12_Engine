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

// Insecure Function.
// 
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

    if (0 < InitialQuadwordCount)
    {
        _mm_stream_si128(Dest + (InitialQuadwordCount - 1), _mm_load_si128(Source + (InitialQuadwordCount - 1)));
        // Fall through
    }

    if (NumQuadwords == InitialQuadwordCount)
    {
        return;
    }

    Dest += InitialQuadwordCount;
    Source += InitialQuadwordCount;
    NumQuadwords -= InitialQuadwordCount;

    size_t CacheLines = NumQuadwords >> 2;

    if (CacheLines)
    {
        _mm_prefetch((char*)(Source + (CacheLines - 1) * 9), _MM_HINT_NTA);    // Fall through

        // Do four quadwords per loop to minimize stalls.
        for (size_t i = CacheLines; 0 < i; --i)
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
    if (NumQuadwords & 3)
    {
        _mm_stream_si128(Dest + (NumQuadwords & 3) - 1, _mm_load_si128(Source + (NumQuadwords & 3) - 1));
        // Fall through
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

inline std::wstring StringToWString(const std::string& str)
{
    wchar_t buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

inline std::wstring AnsiToWString(const char* str)
{
    wchar_t buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str, -1, buffer, 512);
    return std::wstring(buffer);
}

void SetName(ID3D12Object* d3dObject, std::string FileName, std::string FunctionName, size_t LineNumber)
{
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
}