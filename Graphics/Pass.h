#pragma once
#include "stdafx.h"
#include "Graphics.h"
//                                                                                           �������� Buffer (Target)  
//                                                                                           �� ���� RegisteredName
//                                                                                           �� ���� PassName
//                                 Pass �������������������������������������������������������������������������� vector<Sink*>  �������� OutputName
//            ������������������������������������������������������������������������������������������������            ������ vector<Source*> ������ std::string Name
//       BindingPass                                 BufferClearPass                           ���� Buffer
//           ��
//           ��������������������������������������������������������������������������������������������������
//    RenderQueuePass                                FullscreenPass = BlurPass -> FullscreenPass + BlurPass
//           ��                              ����������������������������������������������������������������������
//           ��                     HorizontalBlurPass                 VerticalBlurPass
//           ��������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������            
//  BlurOutlineDrawingPass    MainRenderPass      OutlineDrawingPass    OutlineMaskGenerationPass    ShadowMappingPass     WireframePass

namespace custom
{
	class CommandContext;
}

class Pass
{
public:
	Pass(std::string Name) noexcept;
	virtual ~Pass();

	virtual void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT = 0;
	virtual void RenderWindow() DEBUG_EXCEPT;
	virtual void Reset() DEBUG_EXCEPT;

	std::string GetRegisteredName() const noexcept;
	void SetNumThread(size_t NumThread) { m_NumWorkingThread = NumThread; }
	virtual void finalize();

public:
	bool m_bActive;
	size_t m_PassIndex;
	size_t m_NumWorkingThread;

#ifdef _DEBUG
	float m_DeltaTime = 0.0f;
	float m_DeltaTimeBefore = 0.0f;
#endif
private:
	std::string m_Name;
};