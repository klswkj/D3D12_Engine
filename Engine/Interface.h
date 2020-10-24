#pragma once

#define DLL_EXPORT __declspec(dllexport)

class D3D12Engine;

extern "C"
{
	DLL_EXPORT void		Initialize(D3D12Engine* _Engine);
	// DLL_EXPORT void		LoadSystems(EngineManager* _EngineManager, IAllocator* allocator);
	// DLL_EXPORT void*    CreateGame(IAllocator* allocator);
}