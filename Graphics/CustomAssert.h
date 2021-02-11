#pragma once
// �˾��� �߰�, Output���� ����, txt���� �����Ű��.
// 
// ���ԵǾ���� �� : ����, ����, �Լ�
/*
����� ��:
#include <windows.h> �� �޼����ڽ�,
#include <iostream.h> <ostream.h>�� output�� ���
txt�� ��������

���������� __debugbreak();

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
        printf("%s\n", msg);
        OutputDebugStringA(msg);
        OutputDebugStringA("\n");
    }
    inline void Print(const wchar_t* msg) 
    { 
        wprintf(L"%ws\n", msg);
        OutputDebugStringW(msg);
        OutputDebugStringW(L"\n");
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
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        Print(buffer);
        Print("\n");
        MessageBoxA(nullptr, buffer, 0, 0);
    }
    inline void PrintSubMessage(const wchar_t* format, ...)
    {
        Print("--> ");
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        Print(buffer);
        Print("\n");
        MessageBoxW(nullptr, buffer, 0, 0);
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
        if (FAILED(hr))      \
        {                    \
            AssertInternal::Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ \n" "Line : "  STRINGIFY_BUILTIN(__LINE__) "@" " \n"); \
            AssertInternal::PrintSubMessage("hr = 0x%08X", hr); \
            AssertInternal::PrintSubMessage(__VA_ARGS__); \
            AssertInternal::Print("\n"); \
            __debugbreak(); \
        }
#elif
#define ASSERT(x) ;
#endif