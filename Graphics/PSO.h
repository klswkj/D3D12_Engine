#pragma once
#include "stdafx.h"
#include "RootSignature.h"
#include "RenderingResource.h"
//                PSO
//      忙式式式式式式式式式式扛式式式式式式式式式忖
// GraphicsPSO           ComputePSO

namespace custom
{
    class RootSignature;
    class CommandContext;
    class GraphicsContext;
    class ComputeContext;
}

namespace custom
{
    class PSO
    {
    public:
		PSO()
			:
			m_PSO(nullptr)
        {
        }

        virtual ~PSO() = default;

        static void DestroyAll();

        void SetRootSignature(const RootSignature& BindMappings)
        {
            // m_pRootSignature = &BindMappings;
            m_PersonalRootSignature = BindMappings;
        }
        
        const RootSignature& GetRootSignature() const
        {
            // ASSERT(m_pRootSignature != nullptr);
            ASSERT(m_PersonalRootSignature.GetSignature() != nullptr);
            // return *m_pRootSignature;
            return m_PersonalRootSignature;
        }

        ID3D12PipelineState* GetPipelineStateObject() const { return m_PSO; }

    protected:
#ifdef _DEBUG
        std::wstring m_Name;
#endif
        RootSignature m_PersonalRootSignature;

        // const RootSignature* m_pRootSignature;
        ID3D12PipelineState* m_PSO;
    };
}

class GraphicsPSO final : public custom::PSO, public RenderingResource
{
    friend class CommandContext;
public:
    // Default state is empty.
    GraphicsPSO();
    GraphicsPSO(GraphicsPSO& _GraphicsPSO) noexcept;

    void SetBlendState(const D3D12_BLEND_DESC& BlendDesc);
    void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc);
    void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc);
    void SetSampleMask(const UINT SampleMask);
    void SetPrimitiveTopologyType(const D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
    void SetRenderTargetFormat(const DXGI_FORMAT RTVFormat, const DXGI_FORMAT DSVFormat, const UINT MsaaCount = 1, const UINT MsaaQuality = 0);
    void SetRenderTargetFormats(const UINT NumRTVs, const DXGI_FORMAT* RTVFormats, const DXGI_FORMAT DSVFormat, const UINT MsaaCount = 1, const UINT MsaaQuality = 0);
    void SetInputLayout(const UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* const pInputElementDescs);
    void SetIBStripCutValue(const D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps); // Not used.

    // These const_casts shouldn't be necessary, but we need to fix the API to accept "const void* pShaderBytecode"
    void SetVertexShader(const void* const Binary, const size_t Size);
    void SetPixelShader(const void* const Binary, const size_t Size);
    void SetGeometryShader(const void* const Binary, const size_t Size);
    void SetHullShader(const void* const Binary, const size_t Size);
    void SetDomainShader(const void* const Binary, const size_t Size);

    void SetVertexShader(const D3D12_SHADER_BYTECODE& Binary);
    void SetPixelShader(const D3D12_SHADER_BYTECODE& Binary);
    void SetGeometryShader(const D3D12_SHADER_BYTECODE& Binary);
    void SetHullShader(const D3D12_SHADER_BYTECODE& Binary);
    void SetDomainShader(const D3D12_SHADER_BYTECODE& Binary);

    void operator=(ID3D12RootSignature* const _RootSignature)
    {
        m_PSODesc.pRootSignature = _RootSignature;
    }
    void operator=(const D3D12_BLEND_DESC& _BLEND_DESC)
    {
        m_PSODesc.BlendState = _BLEND_DESC;
    }
    void operator=(const D3D12_RASTERIZER_DESC& _RASTERIZER_DESC)
    {
        m_PSODesc.RasterizerState = _RASTERIZER_DESC;
    }
    void operator=(const D3D12_DEPTH_STENCIL_DESC& _DEPTH_STENCIL_DESC)
    {
        m_PSODesc.DepthStencilState = _DEPTH_STENCIL_DESC;
    }
    void operator=(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& _PRIMITIVE_TOPOLOGY_TYPE)
    {
        ASSERT(_PRIMITIVE_TOPOLOGY_TYPE == D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED);
        m_PSODesc.PrimitiveTopologyType = _PRIMITIVE_TOPOLOGY_TYPE;
    }
    
    ID3D12PipelineState* GetPipelineStateObject() const
    {
        return m_PSO;
    }

    void Bind(custom::CommandContext& baseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;

    // Perform validation and compute a hash value for fast state block comparisons
    void Finalize(const std::wstring& name = L"", const bool ExpectedCollision = false);

private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
    std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_inputLayouts;
  
    struct GraphicsPSOHash
    {
        GraphicsPSOHash() = default;
        void SetHashSeed(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
        {
            pRootSignature = reinterpret_cast<uint64_t>(desc.pRootSignature);
            VS = (desc.VS.BytecodeLength * reinterpret_cast<uint64_t>(desc.VS.pShaderBytecode)) >> 12;
            PS = (desc.PS.BytecodeLength * reinterpret_cast<uint64_t>(desc.PS.pShaderBytecode)) >> 20;
            DS = (desc.DS.BytecodeLength * reinterpret_cast<uint64_t>(desc.GS.pShaderBytecode)) >> 28;
            HS = (desc.HS.BytecodeLength * reinterpret_cast<uint64_t>(desc.HS.pShaderBytecode)) >> 36;

            BlendState = (desc.BlendState.RenderTarget[0].BlendEnable   << 17ull) |
                         (desc.BlendState.RenderTarget[0].LogicOpEnable << 16ull) |
                         (desc.BlendState.RenderTarget[0].BlendOp       << 13ull) |
                         (desc.BlendState.RenderTarget[0].BlendOpAlpha  << 10ull) |
                         (desc.BlendState.RenderTarget[0].SrcBlend      << 5ull)  |
                         (desc.BlendState.RenderTarget[0].DestBlend);

            DepthStencilState = (desc.DepthStencilState.DepthEnable    << 4ull) |
                                (desc.DepthStencilState.DepthWriteMask << 3ull) |
                                (desc.DepthStencilState.DepthFunc       - 1ull);

            RasterizerState =           (desc.RasterizerState.FillMode)              << 30ull           |
                                        (desc.RasterizerState.CullMode)              << 28ull           |
                                        (desc.RasterizerState.MultisampleEnable)     << 27ull           |
                                        (desc.RasterizerState.FrontCounterClockwise) << 26ull           |
           (*reinterpret_cast<uint64_t*>(&desc.RasterizerState.SlopeScaledDepthBias) >> 19ull) << 13ull |
           (*reinterpret_cast<uint64_t*>(&desc.RasterizerState.DepthBias)            >> 19ull);

            InputLayout           = desc.InputLayout.NumElements;
            RTVFormat             = *desc.RTVFormats;
            DSVFormat             = desc.DSVFormat;
            PrimitiveTopologyType = desc.PrimitiveTopologyType;
            NumRenderTargets      = desc.NumRenderTargets;
        }
        union
        {
            // http://timjones.io/blog/archive/2015/09/02/parsing-direct3d-shader-bytecode
            struct
            {
                uint64_t pRootSignature : 8;           // 32 or 64 -> 8    (8)
                uint64_t VS : 8;                       // 4k -> 8          (16)
                uint64_t PS : 8;                       // 4k -> 8          (24)
                uint64_t DS : 8;                       // 4k -> 8          (32)
                uint64_t HS : 8;                       // 4k -> 8          (40)
                uint64_t BlendState : 18;              // 2 + 44 -> 8      (58)
                uint64_t DepthStencilState : 5;        // DepthEnable(1), DepthWriteMask(1), DepthFunc(3) -> 5 (63)
                uint64_t RasterizerState : 31;         // Fillmode(1), CullMode(2), MultiSamplerEnable(1) 
                                                       // FrontCounterClockwise(1), sloperScaledDepthBias(13), DepthBias(13) -> 31 (94)
                uint64_t InputLayout : 8;              // 32 or 64 -> 9    (102)
                uint64_t RTVFormat : 8;                // 9 -> 8           (110)
                uint64_t DSVFormat : 8;                // 9 -> 8           (118)
                uint64_t PrimitiveTopologyType : 3;    // 3 -> 3           (121)
                uint64_t NumRenderTargets : 3;         // 3 -> 3           (124)
                uint64_t _Pad4 : 4;
            };
            struct
            {
                uint64_t Hash0;
                uint64_t Hash1;
            };
        };
    };
    GraphicsPSOHash m_hash;
};

class ComputePSO final : public custom::PSO, public RenderingResource
{
    friend class CommandContext;
public:
    ComputePSO();

    void SetComputeShader(const void* const Binary, const size_t Size)
    { 
        m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); 
    }
    void SetComputeShader(const D3D12_SHADER_BYTECODE& Binary) 
    { 
        m_PSODesc.CS = Binary; 
    }

    void Bind(custom::CommandContext& baseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;

    void Finalize(const std::wstring& name = L"");
private:
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;

    struct ComputePSOHash
    {
        ComputePSOHash() = default;

        void SetHashSeed(D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
        {
            pRootSignature = (0x0000ffff | reinterpret_cast<uint64_t>(desc.pRootSignature));
            pCSByteCode =    (0xffff0000 | reinterpret_cast<uint64_t>(desc.CS.pShaderBytecode));
        }

        union
        {
            struct
            {
                uint64_t pRootSignature : 32;
                uint64_t pCSByteCode : 32;
            };
            struct
            {
                uint64_t Hash0;
            };
        };
    };
    ComputePSOHash m_hash;
};