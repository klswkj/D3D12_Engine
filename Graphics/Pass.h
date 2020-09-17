#pragma once
#include "stdafx.h"

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
	Pass(const char* Name) noexcept;
	virtual ~Pass();

	virtual void Execute(custom::CommandContext&) DEBUG_EXCEPT = 0;
	virtual void Reset() DEBUG_EXCEPT;

	const char* GetRegisteredName() const noexcept;

	virtual void finalize();

public:
	bool m_bActive{ true };
	int32_t m_PassIndex;

private:
	const char* m_Name;
};