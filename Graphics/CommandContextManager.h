#pragma once
#include "CommandContext.h"
#include "ShaderConstantsTypeDefinitions.h"
#include "Camera.h"
#include "ShadowCamera.h"

class CommandContextManager
{
	friend custom::CommandContext;
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

	VSConstants m_VSConstants;
	PSConstants m_PSConstants;
	Camera* m_pCamera{ nullptr };
	ShadowCamera* m_pShadowCamera{ nullptr };
};
