#include "stdafx.h"
#include "CommandContextManager.h"

custom::CommandContext* CommandContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE Type, const std::wstring& ID)
{
	std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);

	auto& AvailableContexts = sm_AvailableContexts[Type];

	custom::CommandContext* ret = nullptr;

	if (AvailableContexts.empty())
	{
		ret = new custom::CommandContext(Type);
		sm_ContextPool[Type].emplace_back(ret);
		ret->Initialize(ID);
	}
	else
	{
		ret = AvailableContexts.front();
		AvailableContexts.pop();
		ret->Reset(ID);
	}

	ASSERT(ret != nullptr);
	ASSERT(ret->m_type == Type);

	ret->m_owningManager = &device::g_commandQueueManager;

	ret->m_Viewport    = m_Viewport;
	ret->m_Rect        = m_Scissor;
	ret->m_VSConstants = m_VSConstants;
	ret->m_PSConstants = m_PSConstants;
	ret->m_pMainCamera = m_pCamera;
	ret->m_pMainLightShadowCamera = m_pShadowCamera;

	return ret;
}

void CommandContextManager::FreeContext(custom::CommandContext* pUsedContext)
{
	ASSERT(pUsedContext != nullptr);
	std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);
	sm_AvailableContexts[pUsedContext->m_type].push(pUsedContext);
}

void CommandContextManager::DestroyAllContexts()
{
	for (size_t i = 0; i < 4; ++i)
	{
		sm_ContextPool[i].clear();
	}
}