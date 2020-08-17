#pragma once
#include "stdafx.h"
#include "SamplerDescriptor.h"

namespace premade
{
    extern custom::SamplerDescriptor g_SamplerLinearWrapDesc;
    extern custom::SamplerDescriptor g_SamplerAnisotropyWrapDesc;
    extern custom::SamplerDescriptor g_SamplerShadowDesc;
    extern custom::SamplerDescriptor g_SamplerLinearClampDesc;
    extern custom::SamplerDescriptor g_SamplerVolumeWrapDesc;
    extern custom::SamplerDescriptor g_SamplerPointClampDesc;
    extern custom::SamplerDescriptor g_SamplerPointBorderDesc;
    extern custom::SamplerDescriptor g_SamplerLinearBorderDesc;

    extern D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerLinearWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerAnisotropyWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerShadow;
    extern D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerLinearClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerVolumeWrap;
    extern D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerPointClamp;
    extern D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerPointBorder;
    extern D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerLinearBorder;

    // Position, TexCoord, Normal, Tangent, Bitangent
    extern D3D12_INPUT_ELEMENT_DESC g_InputElements[];

    extern D3D12_INPUT_ELEMENT_DESC& g_InputElementPosition;
    extern D3D12_INPUT_ELEMENT_DESC& g_InputElementTexcoord;
    extern D3D12_INPUT_ELEMENT_DESC& g_InputElementNormal;
    extern D3D12_INPUT_ELEMENT_DESC& g_InputElementTangent;
    extern D3D12_INPUT_ELEMENT_DESC& g_InputElementBitangent;

    extern D3D12_RASTERIZER_DESC g_RasterizerDefault;    // Counter-clockwise
    extern D3D12_RASTERIZER_DESC g_RasterizerDefaultWire;
    extern D3D12_RASTERIZER_DESC g_RasterizerDefaultMsaa;
    extern D3D12_RASTERIZER_DESC g_RasterizerDefaultCw;    // Clockwise winding
    extern D3D12_RASTERIZER_DESC g_RasterizerDefaultCwMsaa;
    extern D3D12_RASTERIZER_DESC g_RasterizerTwoSided;
    extern D3D12_RASTERIZER_DESC g_RasterizerBackSided;
    extern D3D12_RASTERIZER_DESC g_RasterizerTwoSidedMsaa;
    extern D3D12_RASTERIZER_DESC g_RasterizerShadow;
    extern D3D12_RASTERIZER_DESC g_RasterizerShadowCW;
    extern D3D12_RASTERIZER_DESC g_RasterizerShadowTwoSided;

    extern D3D12_BLEND_DESC g_BlendNoColorWrite;
    extern D3D12_BLEND_DESC g_BlendDisable;
    extern D3D12_BLEND_DESC g_BlendPreMultiplied;
    extern D3D12_BLEND_DESC g_BlendTraditional;
    extern D3D12_BLEND_DESC g_BlendAdditive;
    extern D3D12_BLEND_DESC g_BlendTraditionalAdditive;

    extern D3D12_DEPTH_STENCIL_DESC g_DepthStateDisabled;
    extern D3D12_DEPTH_STENCIL_DESC g_DepthStateReadWrite;
    extern D3D12_DEPTH_STENCIL_DESC g_DepthStateReadOnly;
    extern D3D12_DEPTH_STENCIL_DESC g_DepthStateReadOnlyReversed;
    extern D3D12_DEPTH_STENCIL_DESC g_DepthStateTestEqual;

    void Initialize();
    void Shutdown();
}