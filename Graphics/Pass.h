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

	virtual void Execute(custom::CommandContext&) DEBUG_EXCEPT = 0;
	virtual void RenderWindow() DEBUG_EXCEPT;
	virtual void Reset() DEBUG_EXCEPT;

	std::string GetRegisteredName() const noexcept;

	virtual void finalize();

public:
	bool m_bActive{ true };
	uint32_t m_PassIndex;
#ifdef _DEBUG
	float m_DeltaTime = 0.0f;
#endif
private:
	std::string m_Name;
};