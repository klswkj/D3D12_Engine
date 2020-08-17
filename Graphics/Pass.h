#pragma once
#include "stdafx.h"

//                                                                                           ┌─── Buffer (Target)  
//                                                                                           │ ┌─ RegisteredName
//                                                                                           │ ├─ PassName
//                                 Pass ──────────────────────────────────┬── vector<Sink*>  ┴─┴─ OutputName
//            ┌─────────────────────┴────────────────────────┐            └── vector<Source*> ─┬─ std::string Name
//       BindingPass                                 BufferClearPass                           └─ Buffer
//           │
//           ├───────────────────────────────────────────────┐
//    RenderQueuePass                                FullscreenPass = BlurPass -> FullscreenPass + BlurPass
//           │                              ┌───────────────┴─────────────────┐
//           │                     HorizontalBlurPass                 VerticalBlurPass
//           ├──────────────────────┬────────────────────┬────────────────────────┬────────────────────────┬──────────────────────┐            
//  BlurOutlineDrawingPass    LambertianPass      OutlineDrawingPass    OutlineMaskGenerationPass    ShadowMappingPass     WireframePass

namespace custom
{
	class CommandContext;
}

class Sink;
class Source;

class Pass
{
public:
	Pass(const char* Name) noexcept;
	virtual ~Pass();

	virtual void Execute(custom::CommandContext&) const DEBUG_EXCEPT = 0;
	virtual void Reset() DEBUG_EXCEPT;

	const char* GetRegisteredName() const noexcept;
	const std::vector<std::unique_ptr<Sink>>& GetSinks() const;
	Source& GetSource(const std::string& RegisteredName) const;
	Sink& GetSink(const std::string& RegisteredName) const;

	void SetSinkLinkage(const std::string& RegisteredName, const std::string& Target);
	virtual void finalize();

public:
	bool m_bActive{ true };
	int32_t m_PassIndex;

protected:
	void PushBackSink(std::unique_ptr<Sink> spSink);       // Vector안에 겹치는게 있는지 확인하고 푸시백
	void PushBackSource(std::unique_ptr<Source> spSource); // Vector안에 겹치는게 있는지 확인하고 푸시백
private:
	const char* m_Name;
	std::vector<std::unique_ptr<Sink>> m_Sinks;
	std::vector<std::unique_ptr<Source>> m_Sources;
};