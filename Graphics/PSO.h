#pragma once
//                PSO
//      ��������������������������������������������
// GraphicsPSO           ComputePSO

class RootSignature;

class PSO
{
public:

    PSO() 
        : m_RootSignature(nullptr) 
    {
    }

    static void DestroyAll(void);

    void SetRootSignature(const RootSignature& BindMappings)
    {
        m_RootSignature = &BindMappings;
    }

    const RootSignature& GetRootSignature(void) const
    {
        ASSERT(m_RootSignature != nullptr);
        return *m_RootSignature;
    }

    ID3D12PipelineState* GetPipelineStateObject(void) const 
    { 
        return m_PSO; 
    }

protected:
    const RootSignature* m_RootSignature;
    ID3D12PipelineState* m_PSO;
};

class GraphicsPSO : public PSO
{
    friend class CommandContext;
public:
    // Start with empty state
    GraphicsPSO();

    void SetBlendState(const D3D12_BLEND_DESC& BlendDesc);
    void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc);
    void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc);
    void SetSampleMask(UINT SampleMask);
    void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
    void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
    void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
    void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
    void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

    // These const_casts shouldn't be necessary, but we need to fix the API to accept "const void* pShaderBytecode"
    void SetVertexShader(const void* Binary, size_t Size);
    void SetPixelShader(const void* Binary, size_t Size);
    void SetGeometryShader(const void* Binary, size_t Size);
    void SetHullShader(const void* Binary, size_t Size);
    void SetDomainShader(const void* Binary, size_t Size);

    void SetVertexShader(const D3D12_SHADER_BYTECODE& Binary);
    void SetPixelShader(const D3D12_SHADER_BYTECODE& Binary);
    void SetGeometryShader(const D3D12_SHADER_BYTECODE& Binary);
    void SetHullShader(const D3D12_SHADER_BYTECODE& Binary);
    void SetDomainShader(const D3D12_SHADER_BYTECODE& Binary);

    // Perform validation and compute a hash value for fast state block comparisons
    void Finalize();
private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
    std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_inputLayouts;
  
    struct GraphicsPSOHash
    {
        GraphicsPSOHash() = default;
        void SetHashSeed(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
        {
            pRootSignature = reinterpret_cast<uint64_t>(desc.pRootSignature);
            VS = desc.VS.BytecodeLength;
            PS = desc.PS.BytecodeLength;
            DS = desc.DS.BytecodeLength;
            HS = desc.HS.BytecodeLength;

            BlendState = (desc.BlendState.RenderTarget[0].BlendEnable << 17ull) |
                (desc.BlendState.RenderTarget[0].LogicOpEnable << 16ull)        |
                (desc.BlendState.RenderTarget[0].BlendOp << 13ull)              |
                (desc.BlendState.RenderTarget[0].BlendOpAlpha << 10ull)         |
                (desc.BlendState.RenderTarget[0].SrcBlend << 5ull)              |
                (desc.BlendState.RenderTarget[0].DestBlend);

            DepthStencilState = (desc.DepthStencilState.DepthEnable << 4ull) |
                (desc.DepthStencilState.DepthWriteMask << 3ull)              |
                (desc.DepthStencilState.DepthFunc - 1ull);

            RasterizerState = (desc.RasterizerState.FillMode) << 30ull                                       |
                (desc.RasterizerState.CullMode) << 28ull                                                     |
                (desc.RasterizerState.MultisampleEnable) << 27ull                                            |
                (desc.RasterizerState.FrontCounterClockwise) << 26ull                                        |
                (*reinterpret_cast<uint64_t*>(&desc.RasterizerState.SlopeScaledDepthBias) >> 19ull) << 13ull |
                (*reinterpret_cast<uint64_t*>(&desc.RasterizerState.DepthBias) >> 19ull);

            InputLayout = *reinterpret_cast<uint64_t*>(&desc.InputLayout.pInputElementDescs);
            RTVFormat = *desc.RTVFormats;
            DSVFormat = desc.DSVFormat;
            PrimitiveTopologyType = desc.PrimitiveTopologyType;
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
                uint64_t _Pad7 : 7;
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


class ComputePSO : public PSO
{
    friend class CommandContext;
public:
    ComputePSO();

    void SetComputeShader(const void* Binary, size_t Size) 
    { 
        m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); 
    }
    void SetComputeShader(const D3D12_SHADER_BYTECODE& Binary) 
    { 
        m_PSODesc.CS = Binary; 
    }

    void Finalize();
private:
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;
};