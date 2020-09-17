#pragma once
// 팝업도 뜨고, Output에도 띄우고, txt에도 저장시키자.
// 
// 포함되어야할 것 : 파일, 라인, 함수
/*
사용할 것:
#include <windows.h> 의 메세지박스,
#include <iostream.h> <ostream.h>의 output에 출력
txt에 에러저장

마지막으로 __debugbreak();

*/
#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include <varargs.h>
#include <string>
#include <d3d12.h>
#include <sstream>

namespace AssertInternal
{
    inline void Print(const char* msg) 
    { 
        printf("%s", msg);
        OutputDebugStringA("\n");
        OutputDebugStringA(msg);
        OutputDebugStringA("\n");
        MessageBoxA(nullptr, msg, 0, 0);
    }
    inline void Print(const wchar_t* msg) 
    { 
        wprintf(L"%ws", msg);
        OutputDebugStringW(L"\n");
        OutputDebugStringW(msg);
        OutputDebugStringW(L"\n");
        MessageBoxW(nullptr, msg, 0, 0);
    }

    inline void Printf(const char* format, ...)
    {
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        Print(buffer);
    }

    inline void Printf(const wchar_t* format, ...)
    {
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        Print(buffer);
    }

#ifndef RELEASE
    inline void PrintSubMessage(const char* format, ...)
    {
        // Print("--> ");
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        Print(buffer);
        Print("\n");
    }
    inline void PrintSubMessage(const wchar_t* format, ...)
    {
        // Print("--> ");
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        Print(buffer);
        Print("\n");
    }
    inline void PrintSubMessage(void)
    {
    }
#endif
}

#ifdef ERROR
#undef ERROR
#endif
#ifdef ASSERT
#undef ASSERT
#endif

#if defined(_DEBUG)
#define STRINGIFY(x) #x
#define STRINGIFY_BUILTIN(x) STRINGIFY(x)
#define ASSERT(bFalse, ...)                                              \
        if (!(bool)(bFalse))                                             \
        {                                                                \
            AssertInternal::Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ \n" "Line : "  STRINGIFY_BUILTIN(__LINE__) "@" " \n"); \
            AssertInternal::PrintSubMessage("\'" #bFalse "\' is false"); \
            AssertInternal::PrintSubMessage(__VA_ARGS__);                \
            AssertInternal::Print("\n");                                 \
            __debugbreak();                                              \
        }

#define ASSERT_HR( hr, ... ) \
        if (FAILED(hr)) { \
            AssertInternal::Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ \n" "Line : "  STRINGIFY_BUILTIN(__LINE__) "@" " \n"); \
            AssertInternal::PrintSubMessage("hr = 0x%08X", hr); \
            AssertInternal::PrintSubMessage(__VA_ARGS__); \
            AssertInternal::Print("\n"); \
            __debugbreak(); \
        }
#endif
/*
inline std::wstring AnsiToWString(const std::string& str)
{
    wchar_t buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

void SetName(ID3D12Object* d3dObject, std::string FileName, std::string FunctionName, size_t lineNumber)
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

    name += AnsiToWString(FileName.substr(firstchar + 1));
    name += L", Function : ";
    name += AnsiToWString(FunctionName);
    name += L", Line : ";
    name += std::to_wstring(lineNumber);

    d3dObject->SetName(name.c_str());
}

#if defined (_DEBUG)
#define SET_NAME(x) SetName(x, __FILE__, __FUNCTION__, __LINE__)
#else
#define SET_NAME(X) {}
#endif
*/
/*
std::wstring ToWString(const std::string& str)
{
    std::wstringstream cls;
    cls << str.c_str();
    std::wstring total = cls.str();
    return total;
}
*/