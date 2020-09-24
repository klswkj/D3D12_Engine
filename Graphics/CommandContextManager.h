#pragma once
#include "CommandContext.h"
#include "ShaderConstantsTypeDefinitions.h"
#include "Camera.h"
#include "ShadowCamera.h"

class CommandContextManager
{
	friend custom::CommandContext;
	friend custom::GraphicsContext;
public:
	CommandContextManager()
	{
	}

	custom::CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE Type);

	void FreeContext(custom::CommandContext* pUsedContext);

	void DestroyAllContexts();

private:
	std::vector<std::unique_ptr<custom::CommandContext>> sm_ContextPool[4];
	std::queue<custom::CommandContext*> sm_AvailableContexts[4];
	std::mutex sm_ContextAllocationMutex;

	D3D12_VIEWPORT m_Viewport = {};
	D3D12_RECT m_Scissor = {};
	VSConstants m_VSConstants;
	PSConstants m_PSConstants;
	Camera* m_pCamera{ nullptr };
	ShadowCamera* m_pShadowCamera{ nullptr };
};
