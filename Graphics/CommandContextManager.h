#pragma once
#include "ShaderConstantsTypeDefinitions.h"
#include "CommandContext.h"

class IBaseCamera;
class Camera;
class ShadowCamera;
class MainLight;

namespace custom
{
	class ComputeContext;
	class CopyContext;
}

#pragma warning(push)
#pragma warning(disable : 26495)

class CommandContextManager
{
	friend class CommandContext;
	friend class GraphicsContext;
	// friend class ComputeContext;
	// friend class CopyContext;
	
public:
	CommandContextManager()
		:
		m_MainThreadID(::GetCurrentThreadId()),
		m_GlobalCommandContextID(0ull),
		m_GlobalLastGPUWorkSubmmitedFrameIndex(0ull), // -> Recorded by EndFrame()
		m_GlobalLastGPUWorkCompletedFrameIndex(0ull), // -> ?
		m_NumRecordingGraphicsContexts(0u),
		m_NumRecordingComputeContexts(0u),
		m_NumRecordingCopyContexts(0u),
		m_CS({})
	{
		ASSERT(sm_pCommandContextManager == nullptr);
		sm_pCommandContextManager = this;

		::InitializeCriticalSection(&m_CS);
		listHelper::InitializeListHead(&m_RecordingContextHead);
		listHelper::InitializeListHead(&m_CopyContextHead);

		m_Viewport = {};
		m_Scissor  = {};
		m_pMainCamera       = nullptr;
		m_pMainShadowCamera = nullptr;
	}

	~CommandContextManager() { if (m_CS.DebugInfo) { DeleteCriticalSection(&m_CS); } }

	custom::CommandContext* AllocateContext(const D3D12_COMMAND_LIST_TYPE type, const uint8_t numALs);
	custom::CommandContext* GetRecordingContext(const D3D12_COMMAND_LIST_TYPE type, const uint64_t commandContextID);
	custom::CommandContext* GetRecentContext(const D3D12_COMMAND_LIST_TYPE type);
	void PauseRecording(const D3D12_COMMAND_LIST_TYPE type, custom::CommandContext* const pCommandContext);
	void FreeContext(custom::CommandContext* pUsedContext);

	VSConstants GetVSConstants() const { return m_VSConstants; }

public:
	static void DestroyAllCommandContexts();
	static bool NoPausedCommandContext();

	static Camera* GetMainCamera();

	static void SetModelToProjection(const Math::Matrix4& _ViewProjMatrix);
	static void SetModelToProjectionByCamera(const IBaseCamera& _IBaseCamera);
	static void SetModelToShadow(const Math::Matrix4& _ShadowMatrix);
	static void SetModelToShadowByShadowCamera(ShadowCamera& _ShadowCamera);

	static void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	static void SetMainCamera(Camera& _Camera);
	static void SetMainLightDirection(const Math::Vector3& MainLightDirection);
	static void SetMainLightColor(const Math::Vector3& Color, const float Intensity);
	static void SetAmbientLightColor(const Math::Vector3& Color, const float Intensity);
	static void SetShadowTexelSize(const float TexelSize);
	static void SetTileDimension(const uint32_t MainColorBufferWidth, const uint32_t MainColorBufferHeight, const uint32_t LightWorkGroupSize);
	static void SetSpecificLightIndex(const uint32_t FirstConeLightIndex, const uint32_t FirstConeShadowedLightIndex);
	static void SetPSConstants(const MainLight& _MainLight);

private:
	void NumberingCommandContext(custom::CommandContext& commandContext);

public:
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT     m_Scissor;
	VSConstants    m_VSConstants;
	PSConstants    m_PSConstants;
	Camera* m_pMainCamera;
	ShadowCamera* m_pMainShadowCamera;

private:
	static CommandContextManager* sm_pCommandContextManager;

	std::vector<std::unique_ptr<custom::CommandContext>> m_ContextPool[4];
	std::vector<custom::CommandContext*>                 m_AvailableContexts[4];
	LIST_ENTRY                          m_RecordingContextHead; // Graphics¶û, ComputeContext¸¸
	LIST_ENTRY                          m_CopyContextHead;
	uint64_t                            m_GlobalCommandContextID;
	uint64_t                            m_GlobalLastGPUWorkSubmmitedFrameIndex;
	uint64_t                            m_GlobalLastGPUWorkCompletedFrameIndex;
	uint16_t                            m_NumRecordingGraphicsContexts; // Limit to under 8.
	uint16_t                            m_NumRecordingComputeContexts;
	uint16_t                            m_NumRecordingCopyContexts;
	DWORD                               m_MainThreadID;
	CRITICAL_SECTION                    m_CS;

};

#pragma warning(pop)