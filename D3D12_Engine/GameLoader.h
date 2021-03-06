#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif
#define NOMINMAX

#include <Windows.h>
class D3D12Engine;

class SystemManager;
class IAllocator;
struct EngineContext;

typedef void  (*LoadSystemsFunc)(SystemManager* systemManager, IAllocator* allocator);
typedef void  (*InitializeFunc)(EngineContext* context);
typedef void* (*CreateGameFunc)(IAllocator* allocator);

class GameLoader
{
public:
	void	InitializeLoader(EngineContext* context);
	void	LoadSystems(D3D12Engine* _Engine, IAllocator* allocator);
	D3D12Engine* CreateGame(IAllocator* allocator);
	void	LoadGameLibrary(const char* dll);
	void	FreeGameLibrary();
private:
	LoadSystemsFunc loadSystemsFunc;
	InitializeFunc  initializeFunc;
	CreateGameFunc  createGameFunc;
	HINSTANCE		hInstLib;
	BOOL			result;
};


