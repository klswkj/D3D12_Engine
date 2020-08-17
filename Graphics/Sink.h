#pragma once
#include "stdafx.h"

//                                                                   Sink ── string RegisteredName, PassName, OutputName
//                                                                     │
//                                                                     │                                                        
//            ┌────────────────────────────────────────────────────────┼───────────────────────────────────────────────────────┐
//  DirectBufferSink ┬ Source(Buffer)의 컨테이너의 포인터 복사     ContainerBindableSink ┬ 초기값(Index)                  DirectBindableSink ┬ HorizontalBlurPass와 VerticalBlurPass에
//                   │ Linked되어있는지 확인                                            │ BindingPass의 binds에                             │ CachingPixelContstantBufferEx를 바인딩
//                   └                                                                   Source바인딩함.                                   └
//

class Pass;
class Source;

class Sink
{
public:
	const std::string& GetRegisteredName() const noexcept;
	const std::string& GetPassName() const noexcept;
	const std::string& GetOutputName() const noexcept;
	void SetTarget(std::string passName, std::string outputName);
	virtual void Bind(Source& source) = 0;
	virtual void PostLinkValidate() const = 0;
	virtual ~Sink() = default;
protected:
	Sink(std::string registeredName);
private:
	std::string registeredName;
	std::string passName;
	std::string outputName;
};