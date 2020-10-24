#pragma once
#include "stdafx.h"
#include "Graphics.h"
//                                                                                           忙式式式 Buffer (Target)  
//                                                                                           弛 忙式 RegisteredName
//                                                                                           弛 戍式 PassName
//                                 Pass 式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式成式式 vector<Sink*>  扛式扛式 OutputName
//            忙式式式式式式式式式式式式式式式式式式式式式扛式式式式式式式式式式式式式式式式式式式式式式式式忖            戌式式 vector<Source*> 式成式 std::string Name
//       BindingPass                                 BufferClearPass                           戌式 Buffer
//           弛
//           戍式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式忖
//    RenderQueuePass                                FullscreenPass = BlurPass -> FullscreenPass + BlurPass
//           弛                              忙式式式式式式式式式式式式式式式扛式式式式式式式式式式式式式式式式式忖
//           弛                     HorizontalBlurPass                 VerticalBlurPass
//           戍式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式式式式式成式式式式式式式式式式式式式式式式式式式式式式忖            
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