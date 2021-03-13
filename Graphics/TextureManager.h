#pragma once
#include "stdafx.h"
// #include "Texture.h"
// #include "ManagedTexture.h"

namespace custom
{
    class Texture;
}

class ManagedTexture;

std::wstring MakeWStr(const std::string& str);

namespace TextureManager
{
    void Initialize(const std::wstring TextureLibRoot = L"");
    void Shutdown();

    std::wstring GetRootPath();
    uint64_t AllTextureResourceTransition(const D3D12_RESOURCE_STATES newState);

    std::pair<ManagedTexture*, bool> FindOrLoadTexture(const std::wstring& fileName);
    const ManagedTexture* LoadFromFile(const std::wstring& fileName, const bool bStandardRGB = false);
    const ManagedTexture* LoadDDSFromFile(const std::wstring& fileName, const bool bStandardRGB = false);
    const ManagedTexture* LoadTGAFromFile(const std::wstring& fileName, const bool bStandardRGB = false);
    const ManagedTexture* LoadPIXImageFromFile(const std::wstring& fileName);
    const ManagedTexture* LoadWICFromFile(const std::wstring& fileName, const UINT RootIndex = -1, const UINT Offset = -1, const bool bStandardRGB = false);

    inline const ManagedTexture* LoadFromFile(const std::string& fileName, const bool bStandardRGB = false)
    {
        return LoadFromFile(MakeWStr(fileName), bStandardRGB);
    }

    inline const ManagedTexture* LoadDDSFromFile(const std::string& fileName, const bool bStandardRGB = false)
    {
        return LoadDDSFromFile(MakeWStr(fileName), bStandardRGB);
    }

    inline const ManagedTexture* LoadTGAFromFile(const std::string& fileName, const bool bStandardRGB = false)
    {
        return LoadTGAFromFile(MakeWStr(fileName), bStandardRGB);
    }

    inline const ManagedTexture* LoadPIXImageFromFile(const std::string& fileName)
    {
        return LoadPIXImageFromFile(MakeWStr(fileName));
    }

    inline const ManagedTexture* LoadWICImageFromFile(const std::string& fileName, const bool bStandardRGB = false)
    {
        // modified from const LoadWICImageFromFile(MakeWStr(fileName), bStandardRGB). 
        return LoadWICFromFile(MakeWStr(fileName), bStandardRGB);
    }

    const custom::Texture& GetBlackTex2D();
    const custom::Texture& GetWhiteTex2D();
    const custom::Texture& GetMagentaTex2D();
}
