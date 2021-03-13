#pragma once

#ifndef DEBUG_EXCEPT
#define DEBUG_EXCEPT noexcept(!_DEBUG)
#endif

//          IDisplaySurface, RenderingResource
//            ��������������������������������������������������
//       DepthStencil             RenderTarget

namespace custom
{
	class CommandContext;
}

class IDisplaySurface // BufferResource
{
public:
	virtual ~IDisplaySurface() = default;
	virtual void BindAsBuffer(custom::CommandContext&) DEBUG_EXCEPT = 0;
	virtual void BindAsBuffer(custom::CommandContext&, IDisplaySurface*) DEBUG_EXCEPT = 0;
	virtual void Clear(custom::CommandContext&) DEBUG_EXCEPT = 0;
};
