#include "stdafx.h"
#include "TextureManager.h"
#include "ManagedTexture.h"
#include "CommandContext.h"
#include "CommandContextManager.h"

std::wstring MakeWStr(const std::string& str)
{
	return std::wstring(str.begin(), str.end());
}

namespace TextureManager
{
	std::wstring s_RootPath = L"";
	// std::map<std::wstring, std::unique_ptr<ManagedTexture>> s_TextureCache;
	std::map<std::wstring, std::shared_ptr<ManagedTexture>> s_TextureCache;

	void Initialize(const std::wstring wTextureLibRoot/* = L""*/)
	{
		s_RootPath = wTextureLibRoot;
	}

	uint64_t AllTextureResourceTransition(const D3D12_RESOURCE_STATES newState)
	{
		custom::GraphicsContext& graphicsContext = custom::GraphicsContext::Begin(1u);
		uint64_t numResourceCommand = 0ull;

		for (auto& e : s_TextureCache)
		{
			graphicsContext.TransitionResource(*e.second, newState);

			if (16u <= graphicsContext.GetNumStandbyResourceBarrier())
			{
				graphicsContext.SubmitResourceBarriers(0u);
				++numResourceCommand;
			}
		}

		if (graphicsContext.GetNumStandbyResourceBarrier())
		{
			graphicsContext.SubmitResourceBarriers(0u);
			++numResourceCommand;
		}

		if (numResourceCommand)
		{
			return graphicsContext.Finish(false);
		}
		else
		{
			device::g_commandContextManager.FreeContext(&graphicsContext);
			return 0ull;
		}
	}

	void Shutdown()
	{
		for (auto& e : s_TextureCache)
		{
			e.second->Destroy();
		}
	}

	std::wstring GetRootPath()
	{
		return s_RootPath;
	}

	std::pair<ManagedTexture*, bool> FindOrLoadTexture(const std::wstring& wFileName)
	{
		static std::mutex s_Mutex;
		std::lock_guard<std::mutex> Guard(s_Mutex);

		auto iter = s_TextureCache.find(wFileName);

		// If it's found, it has already been loaded or the load process has begun
		if (iter != s_TextureCache.end())
		{
			return std::make_pair(iter->second.get(), false);
		}

		ManagedTexture* NewTexture = new ManagedTexture(wFileName);
		s_TextureCache[wFileName].reset(NewTexture);

		// This was the first time it was requested, so indicate that the caller must read the file
		return std::make_pair(NewTexture, true);
	}
	
	const custom::Texture& GetBlackTex2D()
	{
		auto ManagedTex = FindOrLoadTexture(L"DefaultBlackTexture");

		ManagedTexture* ManTex = ManagedTex.first;
		const bool RequestsLoad = ManagedTex.second;

		if (!RequestsLoad)
		{
			ManTex->WaitForLoad();
			return *ManTex;
		}

		uint32_t BlackPixel = 0;
		ManTex->CreateCommittedTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &BlackPixel);
		return *ManTex;
	}

	const custom::Texture& GetWhiteTex2D()
	{
		auto ManagedTex = FindOrLoadTexture(L"DefaultWhiteTexture");

		ManagedTexture* ManTex = ManagedTex.first;
		const bool RequestsLoad = ManagedTex.second;

		if (!RequestsLoad)
		{
			ManTex->WaitForLoad();
			return *ManTex;
		}

		uint32_t WhitePixel = 0xFFFFFFFFul;
		ManTex->CreateCommittedTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &WhitePixel);
		return *ManTex;
	}

	const custom::Texture& GetMagentaTex2D()
	{
		auto ManagedTex = FindOrLoadTexture(L"DefaultMagentaTexture");

		ManagedTexture* ManTex = ManagedTex.first;
		const bool RequestsLoad = ManagedTex.second;

		if (!RequestsLoad)
		{
			ManTex->WaitForLoad();
			return *ManTex;
		}

		uint32_t MagentaPixel = 0x00FF00FF;
		ManTex->CreateCommittedTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &MagentaPixel);
		return *ManTex;
	}
} // namespace TextureManager