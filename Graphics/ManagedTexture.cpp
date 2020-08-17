#pragma once
#include "stdafx.h"
#include "FileReader.h"
#include "ManagedTexture.h"
#include "TextureManager.h"

#include "CommandContext.h"
#include "GraphicsContext.h"

void ManagedTexture::WaitForLoad(void) const
{
    volatile D3D12_CPU_DESCRIPTOR_HANDLE& VolHandle = (volatile D3D12_CPU_DESCRIPTOR_HANDLE&)m_hCpuDescriptorHandle;

    volatile bool& VolValid = (volatile bool&)m_IsValid;

	while (VolHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN && VolValid)
	{
		std::this_thread::yield();
	}
}

void ManagedTexture::SetToInvalidTexture(void)
{
    m_hCpuDescriptorHandle = TextureManager::GetMagentaTex2D().GetSRV();
    m_IsValid = false;
}

void ManagedTexture::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
    custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

    // m_ShaderInputFlag를 다른 방법으로 정제해서 사용할지 아니면
    // Texture에서 바인딩을 불러오지않고, Mesh에서 Bind()해서 한번에 
    // graphicsContext.SetDynamicDescriptor(RootIndex, offse, count, D3D12_CPU_DESCRIPTOR_HANDLE*[])로 한번에 할지
    // 후에 결정.
    graphicsContext.SetDynamicDescriptor(m_ShaderInputFlag, 0, m_hCpuDescriptorHandle);
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

const ManagedTexture* TextureManager::LoadWICFromFile(const std::wstring& fileName, bool bStandardRGB/* = false*/)
{
    auto ManagedTex = FindOrLoadTexture(fileName);

    ManagedTexture* ManTex = ManagedTex.first;
    const bool RequestsLoad = ManagedTex.second;

    if (!RequestsLoad)
    {
        ManTex->WaitForLoad();
        return ManTex;
    }

    if (ManTex->CreateWICFromMemory(fileName))
    {
        ManTex->SetToInvalidTexture();
    }

    return ManTex;
}