#pragma once
#include "stdafx.h"
#include "FileReader.h"
#include "ManagedTexture.h"
#include "TextureManager.h"
#include "CommandContext.h"

void ManagedTexture::WaitForLoad() const
{
    volatile D3D12_CPU_DESCRIPTOR_HANDLE& VolHandle = (volatile D3D12_CPU_DESCRIPTOR_HANDLE&)m_hCpuDescriptorHandle;

    volatile bool& VolValid = (volatile bool&)m_IsValid;

	while (VolHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN && VolValid)
	{
		std::this_thread::yield();
	}
}

void ManagedTexture::SetToInvalidTexture()
{
    m_hCpuDescriptorHandle = TextureManager::GetMagentaTex2D().GetSRV();
    m_IsValid = false;
}

const ManagedTexture* TextureManager::LoadFromFile(const std::wstring& fileName, bool bStandardRGB)
{
    std::wstring CatPath = fileName;

    const ManagedTexture* Tex = LoadWICFromFile(CatPath + L".png", bStandardRGB);

    if (!Tex->IsValid())
    {
        LoadWICFromFile(CatPath + L".jpg", bStandardRGB);
    }
    else if (!Tex->IsValid())
    {
        LoadWICFromFile(CatPath + L".bmp", bStandardRGB);
    }
    else if (!Tex->IsValid())
    {
        LoadDDSFromFile(CatPath + L".dds", bStandardRGB);
    }
    else if (!Tex->IsValid())
	{
		Tex = LoadTGAFromFile(CatPath + L".tga", bStandardRGB);
	}
    else if (!Tex->IsValid())
    {
        ASSERT(0, "Invalid Request from ", fileName, ".");
    }

    return Tex;
}

const ManagedTexture* TextureManager::LoadDDSFromFile(const std::wstring& fileName, bool bStandardRGB)
{
    auto ManagedTex = FindOrLoadTexture(fileName);

    ManagedTexture* ManTex = ManagedTex.first;
    const bool RequestsLoad = ManagedTex.second;

    if (!RequestsLoad)
    {
        ManTex->WaitForLoad();
        return ManTex;
    }

    std::shared_ptr<std::vector<byte>> ba = fileReader::ReadFileSync(TextureManager::GetRootPath() + fileName);
	if (ba->size() == 0 || !ManTex->CreateDDSFromMemory(ba->data(), ba->size(), bStandardRGB))
	{
		ManTex->SetToInvalidTexture();
	}
	else
	{
		ManTex->GetResource()->SetName(fileName.c_str());
	}

    return ManTex;
}

const ManagedTexture* TextureManager::LoadTGAFromFile(const std::wstring& fileName, bool bStandardRGB)
{
    auto ManagedTex = FindOrLoadTexture(fileName);

    ManagedTexture* ManTex = ManagedTex.first;
    const bool RequestsLoad = ManagedTex.second;

    if (!RequestsLoad)
    {
        ManTex->WaitForLoad();
        return ManTex;
    }

    std::shared_ptr<std::vector<byte>> ba = fileReader::ReadFileSync(TextureManager::GetRootPath() + fileName);
    if (ba->size() > 0)
    {
        ManTex->CreateTGAFromMemory(ba->data(), ba->size(), bStandardRGB);
        ManTex->GetResource()->SetName(fileName.c_str());
    }
    else
        ManTex->SetToInvalidTexture();

    return ManTex;
}

const ManagedTexture* TextureManager::LoadPIXImageFromFile(const std::wstring& fileName)
{
    auto ManagedTex = FindOrLoadTexture(fileName);

    ManagedTexture* ManTex = ManagedTex.first;
    const bool RequestsLoad = ManagedTex.second;

    if (!RequestsLoad)
    {
        ManTex->WaitForLoad();
        return ManTex;
    }

    std::shared_ptr<std::vector<byte>> ba = fileReader::ReadFileSync(TextureManager::GetRootPath() + fileName);
    if (ba->size())
    {
        ManTex->CreatePIXImageFromMemory(ba->data(), ba->size());
        ManTex->GetResource()->SetName(fileName.c_str());
    }
    else
    {
        ManTex->SetToInvalidTexture();
    }

    return ManTex;
}

const ManagedTexture* TextureManager::LoadWICFromFile(const std::wstring& fileName, UINT RootIndex, UINT Offset, bool bStandardRGB/* = false*/)
{
    std::pair<ManagedTexture*, bool> ManagedTex = FindOrLoadTexture(fileName);

    ManagedTexture* ManTex = ManagedTex.first;
    const bool RequestsLoad = ManagedTex.second;

    if (!RequestsLoad)
    {
        ManTex->WaitForLoad();
        return ManTex;
    }

    if (!ManTex->CreateWICFromMemory(fileName))
    {
        ManTex->SetToInvalidTexture();
    }

    // ManTex->SetRootIndex(RootIndex, Offset);

    return ManTex;
}