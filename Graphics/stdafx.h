#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

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

#include <dxgi1_4.h>
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

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "resource.h" // Icon File

#include "Hash.h"
// #include "WinDefine.h"
#include "TypeDefine.h"
#include "CustomAssert.h"

#define DEBUG_EXCEPT noexcept(!_DEBUG)

#define D3D11
#define D3D12
#define FILEACCESS
#define CONTAINED

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

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
inline std::wstring StringToWString(const std::string& str); // inline std::wstringAnsiToWString(const std::string& str);
inline std::wstring AnsiToWString(const char* str);
void SetName(ID3D12Object* d3dObject, std::string FileName, std::string FunctionName, size_t LineNumber);

#if defined (_DEBUG)
#define SET_NAME(x) SetName(x, __FILE__, __FUNCTION__, __LINE__)
#else
#define SET_NAME(X) {}
#endif
