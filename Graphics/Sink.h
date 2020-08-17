#pragma once
#include "stdafx.h"

//                                                                   Sink ���� string RegisteredName, PassName, OutputName
//                                                                     ��
//                                                                     ��                                                        
//            ������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������
//  DirectBufferSink �� Source(Buffer)�� �����̳��� ������ ����     ContainerBindableSink �� �ʱⰪ(Index)                  DirectBindableSink �� HorizontalBlurPass�� VerticalBlurPass��
//                   �� Linked�Ǿ��ִ��� Ȯ��                                            �� BindingPass�� binds��                             �� CachingPixelContstantBufferEx�� ���ε�
//                   ��                                                                   Source���ε���.                                   ��
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