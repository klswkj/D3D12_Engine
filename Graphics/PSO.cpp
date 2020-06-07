#include "stdafx.h"
#include "Device.h"
#include "PSO.h"
#include "RootSignature.h"

static std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> s_graphicsPSOHashMap;
static std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> s_computePSOHashMap;

void custom::PSO::DestroyAll(void)
{
    s_graphicsPSOHashMap.clear();
    s_computePSOHashMap.clear();
}

GraphicsPSO::GraphicsPSO()
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
    m_PSODesc.SampleMask = 0xFFFFFFFFu;
    m_PSODesc.SampleDesc.Count = 1;
    m_PSODesc.InputLayout.NumElements = 0;
}
void GraphicsPSO::SetVertexShader(const void* Binary, size_t Size)
{
    m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetPixelShader(const void* Binary, size_t Size)
{
    m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetGeometryShader(const void* Binary, size_t Size)
{
    m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetHullShader(const void* Binary, size_t Size)
{
    m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetDomainShader(const void* Binary, size_t Size)
{
    m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetVertexShader(const D3D12_SHADER_BYTECODE& Binary)
{
    m_PSODesc.VS = Binary;
}
void GraphicsPSO::SetPixelShader(const D3D12_SHADER_BYTECODE& Binary)
{
    m_PSODesc.PS = Binary;
}
void GraphicsPSO::SetGeometryShader(const D3D12_SHADER_BYTECODE& Binary)
{
    m_PSODesc.GS = Binary;
}
void GraphicsPSO::SetHullShader(const D3D12_SHADER_BYTECODE& Binary)
{
    m_PSODesc.HS = Binary;
}
void GraphicsPSO::SetDomainShader(const D3D12_SHADER_BYTECODE& Binary)
{
    m_PSODesc.DS = Binary;
}
void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& BlendDesc)
{
    m_PSODesc.BlendState = BlendDesc;
}
void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc)
{
    m_PSODesc.RasterizerState = RasterizerDesc;
}
void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc)
{
    m_PSODesc.DepthStencilState = DepthStencilDesc;
}
void GraphicsPSO::SetSampleMask(UINT SampleMask)
{
    m_PSODesc.SampleMask = SampleMask;
}
void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
    ASSERT(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
    m_PSODesc.PrimitiveTopologyType = TopologyType;
}
void GraphicsPSO::SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
    m_PSODesc.IBStripCutValue = IBProps;
}
void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}
void GraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    ASSERT(NumRTVs == 0 || RTVFormats != nullptr, "Null format array conflicts with non-zero length");
    for (size_t i = 0; i < NumRTVs; ++i)
    {
        m_PSODesc.RTVFormats[i] = RTVFormats[i];
    }
	for (size_t i = NumRTVs; i < m_PSODesc.NumRenderTargets; ++i)
	{
		m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
    m_PSODesc.NumRenderTargets = NumRTVs;
    m_PSODesc.DSVFormat = DSVFormat;
    m_PSODesc.SampleDesc.Count = MsaaCount;
    m_PSODesc.SampleDesc.Quality = MsaaQuality;
}
void GraphicsPSO::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs)
{
    m_PSODesc.InputLayout.NumElements = NumElements;

    if (0 < NumElements)
    {
        D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
        memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
        m_inputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
    }
	else
	{
		m_inputLayouts = nullptr;
	}
}

void GraphicsPSO::Finalize()
{
    // Make sure the root signature is finalized first
    m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
    ASSERT(m_PSODesc.pRootSignature != nullptr);

    m_PSODesc.InputLayout.pInputElementDescs = nullptr;

    m_hash.SetHashSeed(m_PSODesc);
    size_t HashCode = Hash::MakeHash(&m_hash.Hash0);
    m_PSODesc.InputLayout.pInputElementDescs = m_inputLayouts.get();

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static std::mutex s_HashMapMutex;
        std::lock_guard<std::mutex> CS(s_HashMapMutex);
        auto iter = s_graphicsPSOHashMap.find(HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_graphicsPSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_graphicsPSOHashMap[HashCode].GetAddressOf();
        }
		else
		{
			PSORef = iter->second.GetAddressOf();
		}
    }

    if (firstCompile)
    {
        ASSERT_HR(device::g_pDevice->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
        s_graphicsPSOHashMap[HashCode].Attach(m_PSO);
    }
    else
    {
		while (*PSORef == nullptr)
		{
			std::this_thread::yield();
		}
        m_PSO = *PSORef;
    }
}

void ComputePSO::Finalize()
{
    // Make sure the root signature is finalized first
    m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
    ASSERT(m_PSODesc.pRootSignature != nullptr);

    m_hash.SetHashSeed(m_PSODesc);
    size_t HashCode = Hash::MakeHash(&m_hash.Hash0);

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static std::mutex s_HashMapMutex;
        std::lock_guard<std::mutex> CS(s_HashMapMutex);
        auto iter = s_computePSOHashMap.find(HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_computePSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_computePSOHashMap[HashCode].GetAddressOf();
        }
		else
		{
			PSORef = iter->second.GetAddressOf();
		}
    }

    if (firstCompile)
    {
        ASSERT_HR(device::g_pDevice->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
        s_computePSOHashMap[HashCode].Attach(m_PSO);
    }
    else
    {
		while (*PSORef == nullptr)
		{
			std::this_thread::yield();
		}
        m_PSO = *PSORef;
    }
}

ComputePSO::ComputePSO()
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
}
