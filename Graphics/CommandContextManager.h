#pragma once
#include "stdafx.h"

class CommandContextManager
{
public:
	CommandContextManager(void)
	{
	} 

	custom::CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE Type)
	{
		std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);

		auto& AvailableContexts = sm_AvailableContexts[Type];

		custom::CommandContext* ret = nullptr;
		if (AvailableContexts.empty())
		{
			ret = new custom::CommandContext(Type);
			sm_ContextPool[Type].emplace_back(ret);
			ret->Initialize();
		}
		else
		{
			ret = AvailableContexts.front();
			AvailableContexts.pop();
			ret->Reset();
		}
		ASSERT(ret != nullptr);

		ASSERT(ret->m_type == Type);

		return ret;
	}
	void FreeContext(custom::CommandContext* usedContext)
	{
		ASSERT(UsedContext != nullptr);
		std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);
		sm_AvailableContexts[UsedContext->m_type].push(UsedContext);
	}

	void DestroyAllContexts()
	{
		for (size_t i = 0; i < 4; ++i)
		{
			sm_ContextPool[i].clear();
		}
	}

private:
	std::vector<std::unique_ptr<custom::CommandContext> > sm_ContextPool[4];
	std::queue<custom::CommandContext*> sm_AvailableContexts[4];
	std::mutex sm_ContextAllocationMutex;
};
