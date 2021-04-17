#pragma once

#pragma region HEADER

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
// #include <DXProgrammableCapture.h>
#endif

#include <iostream>
#include <sstream>
#include <fstream>

#if 201402L < __cplusplus
// See : https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=vs-2019
#include <filesystem>
#endif

#include <windowsx.h>
#include <Windows.h>
#include <wincodec.h>
#include <wrl.h>
#include <DbgHelp.h>
#include <Tlhelp32.h>
#include <Psapi.h>
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <tchar.h>
#include <ppltasks.h>
#include <exception>
#include <assert.h>
#include <sal.h>

#include <vector>
#include <string>
#include <queue>
#include <set>
#include <map>
#include <unordered_map>

#include <algorithm>
#include <utility>
#include <functional>
#include <memory>
#include <iterator>
#include <optional>

#include <thread>
#include <mutex>

#include <dwrite_2.h>
#include <d2d1_3.h>

#include <dxgiformat.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>
#include <d3d11on12.h>
#include "DDS.h"
#include "DDSTextureLoader.h"

#if defined(_DEBUG) || !defined(RELEASE)
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <pix3.h>
// #include <DXProgrammableCapture.h>
// Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> g_GraphicsAnalysis;
// HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&g_GraphicsAnalysis));
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib, "D3DCompiler.lib")

#include "DirectXMesh.h"
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "resource.h" // Icon File

#include "Hash.h"
#include "ListEntryHelper.h"
// #include "WinDefine.h"
#include "TypeDefine.h"
#include "CustomAssert.h"
#include "D3D12Util.h"

#include "MathCommon.h"
#include "MathBasic.h"
#include "Frustum.h"

//template 
//<typename ToCheckType, size_t LimitedSize, size_t RealSize = sizeof(ToCheckType)>
//void IS_SIZE_OVER()
//{
//    static_assert(RealSize <= LimitedSize, "The Size is off!");
//}

//template <typename ToCheckType>
//void IS_SIZE_OVER2(size_t LimitedSize)
//{
//    static_assert(sizeof(ToCheckType) <= LimitedSize, "The Size is off!");
//}

template<size_t S>
class Sizer
{
};

#pragma endregion HEADER

#pragma region CUSTOM_DEFINE

#define DEBUG_EXCEPT noexcept(!_DEBUG)

#define STATIC
#define D3D11_VER
#define D3D12_VER
#define FILEACCESS
#define CONTAINED

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#define InterlockedGetValue(object) InterlockedCompareExchange(object, 0, 0)

#ifndef USING_THREAD_POOL
#define USING_THREAD_POOL 1
#endif

#if USING_THREAD_POOL
#include "ThreadPool.h"
#endif

#if (_MSC_VER == 1900)
#define FUNCTION_CHECK_RETURN_START __pragma(warning(push)) __pragma(warning(disable: 28216)) __declspec("SAL_checkReturn")
#define FUNCTION_CHECK_RETURN_END __pragma(warning(pop))
#else
#define FUNCTION_CHECK_RETURN_START __declspec("SAL_checkReturn")	/* Warn that callers should not ignore the return value. */
#endif

#ifndef _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING
#endif

#if 1400 <= _MSC_VER
#define OVERRIDE override
#define FINAL    final
#define SEALED   sealed
#else
#define OVERRIDE
#define FINAL
#define SEALED
#endif

#ifndef CHECK_TYPE
#define CHECK_TYPE(Type, FenceValue) (FenceValue >> 56) == (uint64_t)(Type)
#endif
#ifndef CHECK_VALID_FENCE_VALUE
#define CHECK_VALID_FENCE_VALUE(Type, FenceValue) (((uint64_t)Type << 56) <= FenceValue) && \
                                                  (FenceValue < ((uint64_t)(Type + 1ull) << 56))
#endif
#ifndef CHECK_VALID_TYPE
#define CHECK_VALID_TYPE(Type) (Type == D3D12_COMMAND_LIST_TYPE_DIRECT || \
                               Type == D3D12_COMMAND_LIST_TYPE_COMPUTE || \
                               Type == D3D12_COMMAND_LIST_TYPE_COPY)
#endif


#ifndef TYPE_TO_INDEX
#define TYPE_TO_INDEX(Type) ((uint64_t)Type + 1ull) / 2ull
#endif

#pragma endregion CUSTOM_DEFINE

#pragma region CUSTOM_FUNCTION

template<typename T>
inline ULONG SafeRelease(T*& rpInterface)
{
    T* pInterface = rpInterface;
    ULONG ReturnValue = 0ul;
    if (pInterface)
    {
        ReturnValue = pInterface->Release();
        rpInterface = nullptr;
    }

    return ReturnValue;
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

#if defined(_DEBUG)
#define CHECK_IUNKOWN_REF_COUNT_DEBUG(x) \
if(x)                                    \
{                                        \
__debugbreak();                          \
}
#elif
#define CHECK_IUNKOWN_REF_COUNT_DEBUG(x) (x)
#endif

// Insecure Function.
// 
void SIMDMemCopy(void* __restrict _Dest, const void* __restrict _Source, size_t NumQuadwords);
void SIMDMemFill(void* __restrict _Dest, __m128 FillVector, size_t NumQuadwords);

template<class Iter>
void SplitStringIter(const std::string& s, const char* delim, Iter out);
std::vector<std::string> SplitString(const std::string& s, const char* delim);
std::wstring StringToWString(const std::string& str); // inline std::wstringAnsiToWString(const std::string& str);
std::wstring AnsiToWString(const char* str);
void SetName(ID3D12Object* d3dObject, std::string FileName, std::string FunctionName, size_t LineNumber);

std::string _NormalizeString(const std::string& InputString);
std::string SplitFormat(const std::string InputString);
std::wstring _NormalizeWString(const std::wstring& InputString);
std::wstring SplitFormat(const std::wstring InputString);

DirectX::XMFLOAT3 ExtractEulerAngles(const DirectX::XMFLOAT4X4& Matrix);
DirectX::XMFLOAT3 ExtractTranslation(const DirectX::XMFLOAT4X4& Matrix);
DirectX::XMMATRIX ScaleTranslation(DirectX::XMMATRIX Matrix, float Scale);

uint32_t          GetNumLogicalProcessors();

#ifndef RELEASE
#define SET_NAME(x) SetName(x, __FILE__, __FUNCTION__, __LINE__)
#else
#define SET_NAME(X) {}
#endif

#ifndef CRTDEBUG1
#ifndef RELEASE
#define CRTDEBUG1 _CrtMemState s1; \
          _CrtMemCheckpoint(&s1);  \
          HANDLE hLogFile;         \
hLogFile = CreateFile(L"Memory Leaks.txt", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); \
_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);   \
_CrtSetReportFile(_CRT_WARN, hLogFile);            \
_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);  \
_CrtSetReportFile(_CRT_ERROR, hLogFile);           \
_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE); \
_CrtSetReportFile(_CRT_ASSERT, hLogFile);
#else
#define CRTDEBUG1 {}
#endif
#endif

#ifndef CRTDEBUG2
#ifndef RELEASE
#define CRTDEBUG2 _CrtMemState s2;            \
_CrtMemCheckpoint(&s2);                       \
_CrtMemState s3;                              \
if (_CrtMemDifference(&s3, &s1, &s2))         \
{                                             \
    _CrtMemDumpStatistics(&s3);               \
}
#else
#define CRTDEBUG2 {}
#endif
#endif

#pragma endregion CUSTOM_FUNCTION