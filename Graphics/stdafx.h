#pragma once
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
#include <cstdint>
#include <cstdarg>
#include <ppltasks.h>
#include <exception>
#include <assert.h>
#include <sal.h>

#include <vector>
#include <string>
#include <queue>
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
// #include "WinDefine.h"
#include "TypeDefine.h"
#include "CustomAssert.h"

#include "MathCommon.h"
#include "MathBasic.h"

#define DEBUG_EXCEPT noexcept(!_DEBUG)

#define D3D11_VER
#define D3D12_VER
#define FILEACCESS
#define CONTAINED

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#ifndef _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING
#endif

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