#pragma once
#include "stdafx.h"
#include "Graphics.h"

//                                 D3D12Pass 
//                                   ��
//           ��������������������������������������������������������������������������������������������������
//    ID3D12RenderQueuePass                                   ID3D12ScreenPass
//           ��                              ����������������������������������������������������������������������
//           ��                     HorizontalBlurPass                 VerticalBlurPass
//           ��������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������            
//  BlurOutlineDrawingPass    MainRenderPass      OutlineDrawingPass    OutlineMaskGenerationPass    ShadowMappingPass     WireframePass

namespace custom
{
	class CommandContext;
	class GraphicsContext;
	class ComputeContext;
	class CopyContext;
}

enum class ePassShadingTechniqueFlags : bool
{
	eForwardRenderingPass  = 0x0,
	eDeferredRenderingPass = 0x1
};

enum class ePassGPUAsyncFlags : bool
{
	eSingleGPUAsync = 0x0,
	eSingleGPUSync  = 0x1,
};

enum class ePassStageFlags : unsigned char
{
	ePreProcessing  = 0x0,
	eMainProcessing = 0x1,
	ePostProcessing = 0x2
};

struct PassAttributeFlags
{
	ePassShadingTechniqueFlags eTechniqueFlags;
	ePassGPUAsyncFlags         eGPUAsyncFlags;
	ePassStageFlags            eStageFlags;
};

class D3D12Pass
{
public:
	explicit D3D12Pass(std::string Name) DEBUG_EXCEPT;
	virtual ~D3D12Pass();

	// virtual void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT = 0;
	virtual void ExecutePass() DEBUG_EXCEPT = 0;
	virtual void RenderWindow() DEBUG_EXCEPT;
	virtual void Reset() DEBUG_EXCEPT;

	std::string GetRegisteredName() const DEBUG_EXCEPT;
	virtual void finalize() {}

public:
	BOOL               m_bActive;
	PassAttributeFlags m_AttributeFlags;
	size_t             m_PassIndex;

#ifdef _DEBUG
	float m_DeltaTime       = 0.0f;
	float m_DeltaTimeBefore = 0.0f;

	float m_LastGPUExecuteTime = 0.0f;
	float m_GPUDeltaTime       = 0.0f;
#endif
protected:
	std::string m_Name;
};

// class SequentialRenderQueue : public ID3D12RenderQueuePass
// {
// }

class ID3D12STPass : public D3D12Pass
{
public:
	explicit ID3D12STPass(std::string name) : D3D12Pass(name) {}
	virtual ~ID3D12STPass() = default;
};

class ID3D12MTPass : public D3D12Pass
{
public:
	explicit ID3D12MTPass(std::string name) 
		: 
		D3D12Pass(name),
		m_NumTaskFibers(3ul),
		m_NumTransitionResourceState(1u)
	{
	}
	virtual ~ID3D12MTPass() = default;
	// ���� ������Ʈ�ؾߵ���?
	// 1. Scene�� �ٲ� ��,
	// 2. �κ������� �׸� Mesh�� �߰��� ��, �ƴϸ� ���� �� ��,
	// 3. �� �̻� ���µ�.
	// virtual void UpdateNumTaskFibers() = 0; 

	inline uint32_t GetNumTaskFiber() const { ASSERT(1ul < m_NumTaskFibers + m_NumTransitionResourceState); return m_NumTaskFibers + m_NumTransitionResourceState; };

protected:
	uint16_t m_NumTaskFibers; // �۾����� ��� ��������.(= NeedAdditionalCommandALs)
	uint16_t m_NumTransitionResourceState; // -> CommandQueue������ �ƴϰ�, mainThread CommandContext�� �Ű�����.
};

class ID3D12SequentialMTPass : public D3D12Pass
{
public:
	explicit ID3D12SequentialMTPass(std::string name) 
		: 
		D3D12Pass(name),
		m_NumTaskFibers(2ul),
		m_NumTransitionResourceState(0u)
	{
	}
	virtual ~ID3D12SequentialMTPass() = default;
	inline uint32_t GetNumTaskFiber() const { ASSERT(1ul < m_NumTaskFibers + m_NumTransitionResourceState); return m_NumTaskFibers + m_NumTransitionResourceState; };

protected:
	uint16_t m_NumTaskFibers;
	uint16_t m_NumTransitionResourceState; // -> CommandQueue������ �ƴϰ�, mainThread CommandContext�� �Ű�����.
};

// class ID3D12RenderQueuePass : public ID3D12MTPass
class ID3D12ScreenPass : public ID3D12STPass
{
public:
	explicit ID3D12ScreenPass(std::string name) : ID3D12STPass(name) {}
	virtual ~ID3D12ScreenPass() = default;
};

class ID3D12DebugPass : public D3D12Pass
{
public:
	explicit ID3D12DebugPass(std::string name) : D3D12Pass(name) {}
	virtual ~ID3D12DebugPass() = default;
};