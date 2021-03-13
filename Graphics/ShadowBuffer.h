#pragma once
#include "stdafx.h"
#include "DepthBuffer.h"
#include "CommandContext.h"

class ShadowBuffer : public DepthBuffer
{
public:
    ShadowBuffer() 
    {
        ZeroMemory(&m_Viewport, sizeof(m_Viewport));
        ZeroMemory(&m_Scissor, sizeof(m_Scissor));
    }

    void Create(const std::wstring& Name, uint32_t Width, uint32_t Height)
    {
        DepthBuffer::Create(Name, Width, Height, DXGI_FORMAT_D16_UNORM);

        m_Viewport.Width    = (float)Width;
        m_Viewport.Height   = (float)Height;
        m_Viewport.TopLeftX = 0.0f;
        m_Viewport.TopLeftY = 0.0f;
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;

        // Prevent drawing to the boundary pixels so that we don't have to worry about shadows stretching
        m_Scissor.left   = 1l;
        m_Scissor.top    = 1l;
        m_Scissor.right  = (LONG)Width - 2l;
        m_Scissor.bottom = (LONG)Height - 2l;
    }

    void BeginRendering(custom::GraphicsContext& Context)
    {
        ASSERT(false);
        // Context.TransitionResource(*this, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
        // Context.ClearDepth(*this);
        // Context.SetOnlyDepthStencil(GetDSV());
        // Context.SetViewportAndScissor(m_Viewport, m_Scissor);
    }

    void EndRendering(custom::GraphicsContext& Context)
    {
        ASSERT(false);
        // Context.TransitionResource(*this, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const
    {
        return GetDepthSRV();
    }

private:
    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_Scissor;
};