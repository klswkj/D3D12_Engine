#include "stdafx.h"
#include "PreMadePSO.h"
#include "Color.h"

namespace premade
{
    using namespace custom;
    
    SamplerDescriptor g_DefaultSamplerDesc;
    SamplerDescriptor g_SamplerLinearWrapDesc;
    SamplerDescriptor g_SamplerAnisotropyWrapDesc;
    SamplerDescriptor g_SamplerShadowDesc;
    SamplerDescriptor g_SamplerLinearClampDesc;
    SamplerDescriptor g_SamplerVolumeWrapDesc;
    SamplerDescriptor g_SamplerPointClampDesc;
    SamplerDescriptor g_SamplerPointBorderDesc;
    SamplerDescriptor g_SamplerPointMirrorDesc;
    SamplerDescriptor g_SamplerLinearBorderDesc;
    SamplerDescriptor g_SamplerLinearMirrorDesc;


    D3D12_CPU_DESCRIPTOR_HANDLE g_DefaultSampler;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerLinearWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerAnisotropyWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerShadow;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerLinearClamp;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerVolumeWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerPointClamp;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerPointBorder;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerPointMirror;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerLinearBorder;
    D3D12_CPU_DESCRIPTOR_HANDLE g_SamplerLinearMirror;

    D3D12_INPUT_ELEMENT_DESC g_InputElements[5];

    D3D12_INPUT_ELEMENT_DESC& g_InputElementPosition = g_InputElements[0];
    D3D12_INPUT_ELEMENT_DESC& g_InputElementTexcoord = g_InputElements[1];
    D3D12_INPUT_ELEMENT_DESC& g_InputElementNormal = g_InputElements[2];
    D3D12_INPUT_ELEMENT_DESC& g_InputElementTangent = g_InputElements[3];
    D3D12_INPUT_ELEMENT_DESC& g_InputElementBitangent = g_InputElements[4];

    D3D12_RASTERIZER_DESC g_RasterizerDefault;    // Counter-clockwise
    D3D12_RASTERIZER_DESC g_RasterizerDefaultWire;
    D3D12_RASTERIZER_DESC g_RasterizerDefaultMsaa;
    D3D12_RASTERIZER_DESC g_RasterizerDefaultCw;    // Clockwise winding
    D3D12_RASTERIZER_DESC g_RasterizerDefaultCwMsaa;
    D3D12_RASTERIZER_DESC g_RasterizerTwoSided;
    D3D12_RASTERIZER_DESC g_RasterizerBackSided;
    D3D12_RASTERIZER_DESC g_RasterizerTwoSidedMsaa;
    D3D12_RASTERIZER_DESC g_RasterizerShadow;
    D3D12_RASTERIZER_DESC g_RasterizerShadowCW;
    D3D12_RASTERIZER_DESC g_RasterizerShadowTwoSided;
    D3D12_RASTERIZER_DESC g_WireFrameDefault;

    D3D12_BLEND_DESC g_BlendNoColorWrite;
    D3D12_BLEND_DESC g_BlendDisable;
    D3D12_BLEND_DESC g_BlendPreMultiplied;
    D3D12_BLEND_DESC g_BlendTraditional;
    D3D12_BLEND_DESC g_BlendAdditive;
    D3D12_BLEND_DESC g_BlendTraditionalAdditive;

    D3D12_BLEND_DESC g_BlendOutlineDrawing;

    D3D12_DEPTH_STENCIL_DESC g_DepthStateDisabled;
    D3D12_DEPTH_STENCIL_DESC g_DepthStateReadWrite;
    D3D12_DEPTH_STENCIL_DESC g_DepthStateReadOnly;
    D3D12_DEPTH_STENCIL_DESC g_DepthStateReadOnlyReversed;
    D3D12_DEPTH_STENCIL_DESC g_DepthStateTestEqual;

    D3D12_DEPTH_STENCIL_DESC g_DepthStencilWrite;
    D3D12_DEPTH_STENCIL_DESC g_DepthStencilMask;

    void Initialize()
    {
        g_DefaultSamplerDesc.MaxAnisotropy = 8;
        g_DefaultSampler = g_DefaultSamplerDesc.RequestHandle();

        g_SamplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        g_SamplerLinearWrap = g_SamplerLinearWrapDesc.RequestHandle();

        g_SamplerAnisotropyWrapDesc.MaxAnisotropy = 4;
        g_SamplerAnisotropyWrap = g_SamplerAnisotropyWrapDesc.RequestHandle();

        g_SamplerShadowDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        g_SamplerShadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        g_SamplerShadowDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        g_SamplerShadow = g_SamplerShadowDesc.RequestHandle();

        g_SamplerLinearClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        g_SamplerLinearClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        g_SamplerLinearClamp = g_SamplerLinearClampDesc.RequestHandle();

        g_SamplerVolumeWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        g_SamplerVolumeWrap = g_SamplerVolumeWrapDesc.RequestHandle();

        g_SamplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        g_SamplerPointClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
        g_SamplerPointClamp = g_SamplerPointClampDesc.RequestHandle();

        g_SamplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        g_SamplerLinearBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        g_SamplerLinearBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        g_SamplerLinearBorder = g_SamplerLinearBorderDesc.RequestHandle();

        g_SamplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        g_SamplerPointBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        g_SamplerPointBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        g_SamplerPointBorder = g_SamplerPointBorderDesc.RequestHandle();

        g_SamplerPointMirrorDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        g_SamplerPointMirrorDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_MIRROR);
        g_SamplerPointMirrorDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        g_SamplerPointMirror = g_SamplerPointMirrorDesc.RequestHandle();

        g_SamplerLinearMirrorDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        g_SamplerLinearMirrorDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        g_SamplerLinearMirrorDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_MIRROR);
        g_SamplerLinearMirrorDesc.MaxAnisotropy = D3D12_REQ_MAXANISOTROPY;
        g_SamplerLinearMirror = g_SamplerLinearMirrorDesc.RequestHandle();

        // Default rasterizer states
        g_RasterizerDefault.FillMode              = D3D12_FILL_MODE_SOLID;
        g_RasterizerDefault.CullMode              = D3D12_CULL_MODE_BACK;
        g_RasterizerDefault.FrontCounterClockwise = TRUE;
        g_RasterizerDefault.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
        g_RasterizerDefault.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        g_RasterizerDefault.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        g_RasterizerDefault.DepthClipEnable       = TRUE;
        g_RasterizerDefault.MultisampleEnable     = FALSE;
        g_RasterizerDefault.AntialiasedLineEnable = FALSE;
        g_RasterizerDefault.ForcedSampleCount     = 0u;
        g_RasterizerDefault.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        g_RasterizerDefaultWire          = g_RasterizerDefault;
        g_RasterizerDefaultWire.FillMode = D3D12_FILL_MODE_WIREFRAME;

        g_RasterizerDefaultMsaa                   = g_RasterizerDefault;
        g_RasterizerDefaultMsaa.MultisampleEnable = TRUE;

        g_RasterizerDefaultCw                       = g_RasterizerDefault;
        g_RasterizerDefaultCw.FrontCounterClockwise = FALSE;

        g_RasterizerDefaultCwMsaa                   = g_RasterizerDefaultCw;
        g_RasterizerDefaultCwMsaa.MultisampleEnable = TRUE;

        g_RasterizerTwoSided          = g_RasterizerDefault;
        g_RasterizerTwoSided.CullMode = D3D12_CULL_MODE_NONE;

        g_RasterizerBackSided.FillMode              = D3D12_FILL_MODE_SOLID;
        g_RasterizerBackSided.CullMode              = D3D12_CULL_MODE_BACK;
        g_RasterizerBackSided.FrontCounterClockwise = FALSE;
        g_RasterizerBackSided.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
        g_RasterizerBackSided.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        g_RasterizerBackSided.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        g_RasterizerBackSided.DepthClipEnable       = TRUE;
        g_RasterizerBackSided.MultisampleEnable     = FALSE;
        g_RasterizerBackSided.AntialiasedLineEnable = FALSE;
        g_RasterizerBackSided.ForcedSampleCount     = 0u;
        g_RasterizerBackSided.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        g_RasterizerTwoSidedMsaa                   = g_RasterizerTwoSided;
        g_RasterizerTwoSidedMsaa.MultisampleEnable = TRUE;

        // Shadows need their own rasterizer state so we can reverse the winding of faces
        g_RasterizerShadow                      = g_RasterizerDefault;
        //g_RasterizerShadow.CullMode = D3D12_CULL_FRONT;  // Hacked here rather than fixing the content
        g_RasterizerShadow.SlopeScaledDepthBias = -1.5f;
        g_RasterizerShadow.DepthBias            = -100;

        g_RasterizerShadowCW                       = g_RasterizerShadow;
        g_RasterizerShadowCW.FrontCounterClockwise = FALSE;

        g_RasterizerShadowTwoSided          = g_RasterizerShadow;
        g_RasterizerShadowTwoSided.CullMode = D3D12_CULL_MODE_NONE;

        g_WireFrameDefault          = g_RasterizerDefault;
        g_WireFrameDefault.CullMode = D3D12_CULL_MODE_NONE;
        g_WireFrameDefault.FillMode = D3D12_FILL_MODE_WIREFRAME;

        g_DepthStateDisabled.DepthEnable                  = FALSE;
        g_DepthStateDisabled.DepthWriteMask               = D3D12_DEPTH_WRITE_MASK_ZERO;
        g_DepthStateDisabled.DepthFunc                    = D3D12_COMPARISON_FUNC_ALWAYS;
        g_DepthStateDisabled.StencilEnable                = FALSE;
        g_DepthStateDisabled.StencilReadMask              = D3D12_DEFAULT_STENCIL_READ_MASK;
        g_DepthStateDisabled.StencilWriteMask             = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        g_DepthStateDisabled.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
        g_DepthStateDisabled.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
        g_DepthStateDisabled.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
        g_DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        g_DepthStateDisabled.BackFace                     = g_DepthStateDisabled.FrontFace;

        g_DepthStateReadWrite                = g_DepthStateDisabled;
        g_DepthStateReadWrite.DepthEnable    = TRUE;
        g_DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        g_DepthStateReadWrite.DepthFunc      = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

        g_DepthStateReadOnly                = g_DepthStateReadWrite;
        g_DepthStateReadOnly.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

        g_DepthStateReadOnlyReversed           = g_DepthStateReadOnly;
        g_DepthStateReadOnlyReversed.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        g_DepthStateTestEqual           = g_DepthStateReadOnly;
        g_DepthStateTestEqual.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

        // Here
        g_DepthStencilWrite.DepthEnable      = FALSE;
        g_DepthStencilWrite.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
        g_DepthStencilWrite.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ZERO;
        g_DepthStencilWrite.StencilEnable    = TRUE;
        g_DepthStencilWrite.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK;
        g_DepthStencilWrite.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        g_DepthStencilWrite.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP; // Don't care
        g_DepthStencilWrite.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP; // Set Before Value
        g_DepthStencilWrite.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS; // Don't care
        g_DepthStencilWrite.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_REPLACE; // Set to 0xff
        g_DepthStencilWrite.BackFace                     = g_DepthStencilWrite.FrontFace;

        g_DepthStencilMask = g_DepthStencilWrite;
        g_DepthStencilMask.FrontFace.StencilFunc   = D3D12_COMPARISON_FUNC_NOT_EQUAL;
        g_DepthStencilMask.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        g_DepthStencilMask.BackFace                = g_DepthStencilMask.FrontFace;

        D3D12_BLEND_DESC alphaBlend = {};
        alphaBlend.IndependentBlendEnable                = FALSE;
        alphaBlend.RenderTarget[0].BlendEnable           = FALSE;
        alphaBlend.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
        alphaBlend.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
        alphaBlend.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
        alphaBlend.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
        alphaBlend.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
        alphaBlend.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0;

        g_BlendNoColorWrite                              = alphaBlend;

        alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        g_BlendDisable                                   = alphaBlend;

        alphaBlend.RenderTarget[0].BlendEnable = TRUE;
        g_BlendTraditional                     = alphaBlend;

        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        g_BlendPreMultiplied                = alphaBlend;

        alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        g_BlendAdditive                      = alphaBlend;

        alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        g_BlendTraditionalAdditive          = alphaBlend;

        g_BlendOutlineDrawing.AlphaToCoverageEnable  = FALSE;
        g_BlendOutlineDrawing.IndependentBlendEnable = FALSE;
        g_BlendOutlineDrawing.RenderTarget[0].BlendEnable           = TRUE;
        g_BlendOutlineDrawing.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;     // D3D12_BLEND_ONE
        g_BlendOutlineDrawing.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA; // D3D12_BLEND_ZERO
        g_BlendOutlineDrawing.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
        g_BlendOutlineDrawing.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
        g_BlendOutlineDrawing.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_ZERO;
        g_BlendOutlineDrawing.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        g_BlendOutlineDrawing.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        for (size_t i = 1; i < _countof(g_BlendOutlineDrawing.RenderTarget); ++i)
        {
            g_BlendOutlineDrawing.RenderTarget[i] = g_BlendOutlineDrawing.RenderTarget[0];
        }

        g_InputElements[0] = g_InputElementPosition  = { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        g_InputElements[1] = g_InputElementTexcoord  = { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        g_InputElements[2] = g_InputElementNormal    = { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        g_InputElements[3] = g_InputElementTangent   = { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        g_InputElements[4] = g_InputElementBitangent = { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    }

    void Shutdown() 
    {
    }
}