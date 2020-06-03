#pragma once

std::wstring MakeWStr(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}

class ManagedTexture : public custom::Texture
{
public:
    ManagedTexture(ID3D12Device* m_Device, custom::CommandContext& CommandContext, DescriptorHeapManager& DescriptorHeapManager, const std::wstring& FileName)
        : g_pDevice(m_Device), m_rCommandContext(CommandContext), m_rDescriptorHeapManager(DescriptorHeapManager), m_MapKey(FileName), m_IsValid(true)
    {
    }

    void operator= (const custom::Texture& Texture);

    void WaitForLoad(void) const;
    void Unload(void);

    void SetToInvalidTexture(void);
    bool IsValid(void) const { return m_IsValid; }

private:
    std::wstring m_MapKey;        // For deleting from the map later
    bool m_IsValid;
};

namespace TextureManager
{
    void Initialize(const std::wstring& TextureLibRoot);
    void Shutdown(void);

    const ManagedTexture* LoadFromFile(const std::wstring& fileName, bool sRGB = false);
    const ManagedTexture* LoadDDSFromFile(const std::wstring& fileName, bool sRGB = false);
    const ManagedTexture* LoadTGAFromFile(const std::wstring& fileName, bool sRGB = false);
    const ManagedTexture* LoadPIXImageFromFile(const std::wstring& fileName);

    inline const ManagedTexture* LoadFromFile(const std::string& fileName, bool sRGB = false)
    {
        return LoadFromFile(MakeWStr(fileName), sRGB);
    }

    inline const ManagedTexture* LoadDDSFromFile(const std::string& fileName, bool sRGB = false)
    {
        return LoadDDSFromFile(MakeWStr(fileName), sRGB);
    }

    inline const ManagedTexture* LoadTGAFromFile(const std::string& fileName, bool sRGB = false)
    {
        return LoadTGAFromFile(MakeWStr(fileName), sRGB);
    }

    inline const ManagedTexture* LoadPIXImageFromFile(const std::string& fileName)
    {
        return LoadPIXImageFromFile(MakeWStr(fileName));
    }

    const custom::Texture& GetBlackTex2D(void);
    const custom::Texture& GetWhiteTex2D(void);
}
