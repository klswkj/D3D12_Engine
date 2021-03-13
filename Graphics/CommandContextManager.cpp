#include "stdafx.h"
#include "CommandContextManager.h"
// #include "CommandContext.h"
#include "IBaseCamera.h"
#include "Camera.h"
#include "ShadowCamera.h"
#include "MainLight.h"

#pragma warning( push )
#pragma warning( disable : 26454 )

CommandContextManager* CommandContextManager::sm_pCommandContextManager = nullptr;

custom::CommandContext* CommandContextManager::AllocateContext
(
	const D3D12_COMMAND_LIST_TYPE type,
	const uint8_t numALs
)
{
	ASSERT(CHECK_VALID_TYPE(type));
	custom::CommandContext* ReturnContext = nullptr;

	::EnterCriticalSection(&m_CS);
	{
		std::vector<custom::CommandContext*>& AvailableContexts = m_AvailableContexts[type];

		if (AvailableContexts.empty())
		{
			ReturnContext = new custom::CommandContext(type, numALs);
			ReturnContext->m_pOwningManager = &device::g_commandQueueManager;
			NumberingCommandContext(*ReturnContext);
			m_ContextPool[type].emplace_back(ReturnContext);
		}
		else
		{
			custom::CommandContext* ProperContext = nullptr;
			size_t ProperContextIndex = 0;
			int AbsDifference = UCHAR_MAX;

			for (size_t i = 0; i < AvailableContexts.size(); ++i)
			{
				if (AvailableContexts[i])
				{
					int Differ = std::abs((int)AvailableContexts[i]->m_NumCommandPair - (int)numALs);
					
					if (Differ < AbsDifference)
					{
						ProperContext = AvailableContexts[i];
						AbsDifference = Differ;
						ProperContextIndex = i;
					}

					if (Differ == 0)
					{
						break;
					}
				}
			}

			ReturnContext = ProperContext;
			AvailableContexts[ProperContextIndex] = nullptr;
		}
	}

	ASSERT((ReturnContext != nullptr) && 
		(ReturnContext->m_type == type));

	listHelper::InsertTailList((type == D3D12_COMMAND_LIST_TYPE_COPY)? &m_CopyContextHead : &m_RecordingContextHead, &ReturnContext->m_ContextEntry);

	::LeaveCriticalSection(&m_CS);
	
	ReturnContext->Reset(type, numALs);
	ReturnContext->m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

	ReturnContext->m_Viewport    = m_Viewport;
	ReturnContext->m_Rect        = m_Scissor;
	ReturnContext->m_VSConstants = m_VSConstants;
	ReturnContext->m_PSConstants = m_PSConstants;

	ReturnContext->m_pMainCamera            = sm_pCommandContextManager->m_pMainCamera;
	ReturnContext->m_pMainLightShadowCamera = sm_pCommandContextManager->m_pMainShadowCamera;

	return ReturnContext;
}

custom::CommandContext* CommandContextManager::GetRecordingContext(const D3D12_COMMAND_LIST_TYPE type, const uint64_t commandContextID)
{
	ASSERT(CHECK_VALID_TYPE(type));
	custom::CommandContext* pContext = nullptr;
	LIST_ENTRY* pHead = (type == D3D12_COMMAND_LIST_TYPE_COPY) ? &m_CopyContextHead : &m_RecordingContextHead;
	LIST_ENTRY* pTraverse = pHead->Flink;

	::EnterCriticalSection(&m_CS);

	while (pHead != pTraverse)
	{
		pContext = CONTAINING_RECORD(pTraverse, custom::CommandContext, custom::CommandContext::m_ContextEntry);
		if ((pContext->m_CommandContextID == commandContextID) && !(pContext->m_bRecordingFlag >> 63))
		{
			break;
		}

		pTraverse = pTraverse->Flink;
	}

	::LeaveCriticalSection(&m_CS);

	ASSERT(pContext != nullptr);

	pContext->m_bRecordingFlag |= (1ull < 63);

	return pContext;
}
custom::CommandContext* CommandContextManager::GetRecentContext(const D3D12_COMMAND_LIST_TYPE type)
{
	ASSERT(CHECK_VALID_TYPE(type));
	custom::CommandContext* pContext = nullptr;
	LIST_ENTRY* pHead = (type == D3D12_COMMAND_LIST_TYPE_COPY) ? &m_CopyContextHead : &m_RecordingContextHead;
	LIST_ENTRY* pTraverse = pHead->Flink;

	::EnterCriticalSection(&m_CS);

	while (pHead != pTraverse)
	{
		pContext = CONTAINING_RECORD(pTraverse, custom::CommandContext, custom::CommandContext::m_ContextEntry);
		if ((pContext->m_type == type) && !(pContext->m_bRecordingFlag >> 63))
		{
			break;
		}

		pTraverse = pTraverse->Flink;
	}

	ASSERT(pContext != nullptr);

	listHelper::RemoveEntryList(&pContext->m_ContextEntry);
	listHelper::InsertHeadList(pHead, &pContext->m_ContextEntry);

	::LeaveCriticalSection(&m_CS);

	pContext->m_bRecordingFlag |= (1ull < 63);

	return pContext;
}
void CommandContextManager::PauseRecording(const D3D12_COMMAND_LIST_TYPE type, custom::CommandContext* const pCommandContext)
{
	ASSERT(CHECK_VALID_TYPE(type));
	LIST_ENTRY* pHead = (type == D3D12_COMMAND_LIST_TYPE_COPY) ? &m_CopyContextHead : &m_RecordingContextHead;

	::EnterCriticalSection(&m_CS);
	listHelper::RemoveEntryList(&pCommandContext->m_ContextEntry);
	listHelper::InsertTailList(pHead, &pCommandContext->m_ContextEntry);
	::LeaveCriticalSection(&m_CS);

	pCommandContext->m_bRecordingFlag &= ((1ull << 63) - 1ull);
}
void CommandContextManager::NumberingCommandContext(custom::CommandContext& commandContext)
{
	commandContext.m_CommandContextID = m_GlobalCommandContextID++;
}
void CommandContextManager::FreeContext(custom::CommandContext* pUsedContext)
{
	ASSERT(pUsedContext != nullptr);

	::EnterCriticalSection(&m_CS);
	listHelper::RemoveEntryList(&(pUsedContext->m_ContextEntry));
	
	for (auto& e : m_AvailableContexts[pUsedContext->m_type])
	{
		if (e == nullptr)
		{
			e = pUsedContext;
			pUsedContext = nullptr;
			break;
		}
	}

	if (pUsedContext)
	{
		m_AvailableContexts[pUsedContext->m_type].push_back(pUsedContext);
	}

	::LeaveCriticalSection(&m_CS);
}

STATIC bool CommandContextManager::NoPausedCommandContext()
{
	return (sm_pCommandContextManager->m_RecordingContextHead.Flink == 
		   &sm_pCommandContextManager->m_RecordingContextHead);
}
STATIC void CommandContextManager::DestroyAllCommandContexts()
{
	for (size_t i = 0; i < _countof(sm_pCommandContextManager->m_ContextPool); ++i)
	{
		sm_pCommandContextManager->m_ContextPool[i].clear();
	}
}

#pragma region STATIC_FUNCTION


STATIC Camera* CommandContextManager::GetMainCamera()
{
	return sm_pCommandContextManager->m_pMainCamera;
}

STATIC void CommandContextManager::SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	sm_pCommandContextManager->m_Viewport = vp;
	sm_pCommandContextManager->m_Scissor = rect;;
}
STATIC void CommandContextManager::SetModelToProjection(const Math::Matrix4& _ViewProjMatrix)
{
	sm_pCommandContextManager->m_VSConstants.modelToProjection = _ViewProjMatrix;
}
STATIC void CommandContextManager::SetModelToProjectionByCamera(const IBaseCamera& _IBaseCamera)
{
	sm_pCommandContextManager->m_VSConstants.modelToProjection = _IBaseCamera.GetViewProjMatrix();
	sm_pCommandContextManager->m_VSConstants.ViewMatrix = _IBaseCamera.GetViewMatrix();
	DirectX::XMStoreFloat3(&sm_pCommandContextManager->m_VSConstants.viewerPos, _IBaseCamera.GetPosition());
}
STATIC void CommandContextManager::SetModelToShadow(const Math::Matrix4& _ShadowMatrix)
{
	sm_pCommandContextManager->m_VSConstants.modelToShadow = _ShadowMatrix;
}
STATIC void CommandContextManager::SetModelToShadowByShadowCamera(ShadowCamera& _ShadowCamera)
{
	sm_pCommandContextManager->m_pMainShadowCamera = &_ShadowCamera;
	SetModelToShadow(sm_pCommandContextManager->m_pMainShadowCamera->GetShadowMatrix());
}
STATIC void CommandContextManager::SetMainCamera(Camera& _Camera)
{
	sm_pCommandContextManager->m_pMainCamera = &_Camera;
	SetModelToProjectionByCamera(_Camera);
}
STATIC void CommandContextManager::SetMainLightDirection(const Math::Vector3& MainLightDirection)
{
	sm_pCommandContextManager->m_PSConstants.sunDirection = MainLightDirection;
}
STATIC void CommandContextManager::SetMainLightColor(const Math::Vector3& Color, const float Intensity)
{
	sm_pCommandContextManager->m_PSConstants.sunLightColor = Color * Intensity;
}
STATIC void CommandContextManager::SetAmbientLightColor(const Math::Vector3& Color, const float Intensity)
{
	sm_pCommandContextManager->m_PSConstants.ambientColor = Color * Intensity;
}
STATIC void CommandContextManager::SetShadowTexelSize(const float TexelSize)
{
	ASSERT(0.0f < TexelSize && TexelSize < 1.0f);
	sm_pCommandContextManager->m_PSConstants.ShadowTexelSize[0] = TexelSize;
	sm_pCommandContextManager->m_PSConstants.ShadowTexelSize[1] = TexelSize;
}
STATIC void CommandContextManager::SetTileDimension(const uint32_t MainColorBufferWidth, const uint32_t MainColorBufferHeight, const uint32_t LightWorkGroupSize)
{
	ASSERT(1u < LightWorkGroupSize);

	sm_pCommandContextManager->m_PSConstants.InvTileDim[0] = 1.0f / LightWorkGroupSize;
	sm_pCommandContextManager->m_PSConstants.InvTileDim[1] = sm_pCommandContextManager->m_PSConstants.InvTileDim[0];

	sm_pCommandContextManager->m_PSConstants.TileCount[0] = HashInternal::DivideByMultiple(MainColorBufferWidth, LightWorkGroupSize);
	sm_pCommandContextManager->m_PSConstants.TileCount[0] = HashInternal::DivideByMultiple(MainColorBufferHeight, LightWorkGroupSize);
}
STATIC void CommandContextManager::SetSpecificLightIndex(const uint32_t FirstConeLightIndex, const uint32_t FirstConeShadowedLightIndex)
{
	sm_pCommandContextManager->m_PSConstants.FirstLightIndex[0] = FirstConeLightIndex;
	sm_pCommandContextManager->m_PSConstants.FirstLightIndex[1] = FirstConeShadowedLightIndex;
}
STATIC void CommandContextManager::SetPSConstants(const MainLight& _MainLight)
{
	sm_pCommandContextManager->SetMainLightDirection(_MainLight.GetMainLightDirection());
	sm_pCommandContextManager->SetMainLightColor(_MainLight.GetMainLightColor(), _MainLight.GetMainLightIntensity());
	sm_pCommandContextManager->SetAmbientLightColor(_MainLight.GetAmbientColor(), _MainLight.GetAmibientIntensity());
}
#pragma endregion STATIC_FUNCTION

#pragma warning( pop )