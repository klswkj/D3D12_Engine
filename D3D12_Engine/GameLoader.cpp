#include "GameLoader.h"
#include "../Graphics/D3D12Engine.h"

void GameLoader::InitializeLoader(EngineContext* context)
{
	initializeFunc(context);
}

void GameLoader::LoadSystems(D3D12Engine* game, IAllocator* allocator)
{
	auto manager = game->GetGameSystemsManager();
	manager->Destroy();
	loadSystemsFunc(manager, allocator);
	manager->Initialize();
}

D3D12Engine* GameLoader::CreateGame(IAllocator* allocator)
{
	return (D3D12Engine*)createGameFunc(allocator);
}

void GameLoader::LoadGameLibrary(const char* dll)
{
	hInstLib = LoadLibraryA(dll);
	if (hInstLib)
	{
		loadSystemsFunc = (LoadSystemsFunc)GetProcAddress(hInstLib, "LoadSystems");
		initializeFunc = (InitializeFunc)GetProcAddress(hInstLib, "Initialize");
		createGameFunc = (CreateGameFunc)GetProcAddress(hInstLib, "CreateGame");
	}
}

void GameLoader::FreeGameLibrary()
{
	if (hInstLib != NULL)
	{
		result = FreeLibrary(hInstLib);
	}
}
