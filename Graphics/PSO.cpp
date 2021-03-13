#include "stdafx.h"
#include "Device.h"
#include "PSO.h"
// #include "RootSignature.h"
#include "CommandContext.h"
#include "ComputeContext.h"

static std::unordered_map<size_t, ID3D12PipelineState*> s_graphicsPSOHashMap;
static std::unordered_map<size_t, ID3D12PipelineState*> s_computePSOHashMap;

namespace custom
{
    void PSO::DestroyAll()
    {
        for (auto& e : s_graphicsPSOHashMap)
        {
            e.second->Release();
            e.second = nullptr;
        }
        for (auto& e : s_computePSOHashMap)
        {
            e.second->Release();
            e.second = nullptr;
        }

        s_graphicsPSOHashMap.clear();
        s_computePSOHashMap.clear();
    }
}

GraphicsPSO::GraphicsPSO()
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
    m_PSODesc.SampleMask = 0xFFFFFFFFu;
    m_PSODesc.SampleDesc.Count = 1;
    m_PSODesc.InputLayout.NumElements = 0;
}

GraphicsPSO::GraphicsPSO(GraphicsPSO& _GraphicsPSO) noexcept
{
    // m_pRootSignature        = &_GraphicsPSO.GetRootSignature();
    m_PersonalRootSignature = _GraphicsPSO.GetRootSignature();
    m_PSO                   = _GraphicsPSO.m_PSO;
    m_PSODesc               = _GraphicsPSO.m_PSODesc;
    m_inputLayouts          = _GraphicsPSO.m_inputLayouts;
    m_hash                  = _GraphicsPSO.m_hash;
}

ComputePSO::ComputePSO()
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
}

#pragma region GraphicsPSOSetter

void GraphicsPSO::SetVertexShader(const void* const Binary, const size_t Size)
{
    m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetPixelShader(const void* const Binary, const size_t Size)
{
    m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetGeometryShader(const void* const Binary, const size_t Size)
{
    m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetHullShader(const void* const Binary, const size_t Size)
{
    m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size);
}
void GraphicsPSO::SetDomainShader(const void* const Binary, const size_t Size)
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
void GraphicsPSO::SetSampleMask(const UINT SampleMask)
{
    m_PSODesc.SampleMask = SampleMask;
}
void GraphicsPSO::SetPrimitiveTopologyType(const D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
    ASSERT(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
    m_PSODesc.PrimitiveTopologyType = TopologyType;
}
void GraphicsPSO::SetRenderTargetFormat(const DXGI_FORMAT RTVFormat, const DXGI_FORMAT DSVFormat, const UINT MsaaCount /* = 1*/, const UINT MsaaQuality /* = 0*/)
{
    SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}
void GraphicsPSO::SetRenderTargetFormats(const UINT NumRTVs, const DXGI_FORMAT* const RTVFormats, const DXGI_FORMAT DSVFormat, const UINT MsaaCount /* = 1*/, const UINT MsaaQuality /* = 0*/)
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
    m_PSODesc.NumRenderTargets   = NumRTVs;
    m_PSODesc.DSVFormat          = DSVFormat;
    m_PSODesc.SampleDesc.Count   = MsaaCount;
    m_PSODesc.SampleDesc.Quality = MsaaQuality;
}
void GraphicsPSO::SetInputLayout(const UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* const pInputElementDescs)
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
void GraphicsPSO::SetIBStripCutValue(const D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
    m_PSODesc.IBStripCutValue = IBProps;
}
#pragma endregion GraphicsPSOSetter

void GraphicsPSO::Bind(custom::CommandContext& baseContext, const uint8_t commandIndex)
{
    baseContext.SetPipelineState(*this, commandIndex);
}
void ComputePSO::Bind(custom::CommandContext& baseContext, const uint8_t commandIndex)
{
    baseContext.SetPipelineState(*this, commandIndex);
}

void GraphicsPSO::Finalize(const std::wstring& Name, const bool ExpectedCollision)
{
    ASSERT(2 <= Name.size());
    // Make sure the root signature is finalized first
    // m_PSODesc.pRootSignature = m_pRootSignature->GetSignature();
    m_PSODesc.pRootSignature = m_PersonalRootSignature.GetSignature();
    ASSERT(m_PSODesc.pRootSignature != nullptr);

    m_PSODesc.InputLayout.pInputElementDescs = nullptr;

    m_hash.SetHashSeed(m_PSODesc);
    size_t HashCode = Hash::MakeHash(&m_hash.Hash0, 2ull);
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
            PSORef = &s_graphicsPSOHashMap[HashCode];
        }
		else
		{
			PSORef = &iter->second;
		}
    }

    if (firstCompile)
    {
        HRESULT hardwareResult = S_FALSE;

        ASSERT_HR(hardwareResult = device::g_pDevice->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
        s_graphicsPSOHashMap[HashCode] = m_PSO;

#ifdef _DEBUG
        m_PSO->SetName(Name.c_str());
        m_Name = Name;
#endif
    }
    else
    {
		while (*PSORef == nullptr)
		{
			std::this_thread::yield();
		}
        m_PSO = *PSORef;
#ifdef _DEBUG
        wchar_t name[128] = {};
        UINT size = sizeof(name);
        (*PSORef)->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
        m_PSO->SetName(name);

        m_Name.clear();

        if (Name.size())
        {
            m_Name += Name;
            m_Name += L" is Overwritten by ";
        }

        m_Name += name;
#endif
    }
}
void ComputePSO::Finalize(const std::wstring& Name)
{
    ASSERT(2 <= Name.size());

    // Make sure the root signature is finalized first
    // m_PSODesc.pRootSignature = m_pRootSignature->GetSignature();
    m_PSODesc.pRootSignature = m_PersonalRootSignature.GetSignature();
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
            PSORef = &s_computePSOHashMap[HashCode];
        }
		else
		{
			PSORef = &iter->second;
		}
    }

    if (firstCompile)
    {
        static HRESULT hardwareResult;
        ASSERT_HR(hardwareResult = device::g_pDevice->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
        s_computePSOHashMap[HashCode] = m_PSO; 

#ifdef _DEBUG
            m_PSO->SetName(Name.c_str());
            m_Name = Name;
#elif
        Name;
#endif
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

