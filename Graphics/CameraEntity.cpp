#include "CameraEntity.h"
#include "PreMadePSO.h"
#include "BufferManager.h"
#include "RootSignature.h"
#include "PSO.h"
#include "UAVBuffer.h"

#include "Technique.h"
#include "Vector.h"
#include "VariableConstantBuffer.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GeneralPurposePS.h"   // Pseudo Shader
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GeneralPurposeVS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/GeneralPurposePS.h" // Pseudo Shader
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/GeneralPurposeVS.h"
#endif

CameraEntity::CameraEntity()
{
	// x = 1.0f; y = 0.75f; z  -2.0f;
	// tHalf = 0.5f; tSpace = 0.15f

	DirectX::XMFLOAT3 vertices[8] = { {-1.0f, 0.75f, 0.0f},
			                          {1.0f, 0.75f, 0.0f},
			                          {1.0f, 0.75f, 0.0f,},
			                          {-1.0f, -0.75f, 0.0f},
			                          {0.0f, 0.0f, -2.0f},
	                                  {-0.5f, 0.75f + 0.5f, 0.0f},
	                                  {0.5f, 0.75f + 0.5f, 0.0f},
	                                  {0.0f, 0.5f + 0.15f, 0.0f} };

	uint8_t indices[22] = 
	{   
		0, 1,
		1, 2,
		2, 3,
		3, 0,
		0, 4,
		1, 4,
		2, 4,
		3, 4,
		5, 6,
		6, 7,
		7, 5
	};

	m_VerticesBuffer.Create(L"Camera Entity VertexBuffer", _countof(vertices), sizeof(DirectX::XMFLOAT3), vertices);
	m_IndicesBuffer.Create(L"Camera Entity IndexBuffer", _countof(indices), sizeof(uint8_t), indices);

	Technique DrawLine{ "Draw Camera", eObjectFilterFlag::kOpaque };
	Step step("Lamertian");

	m_RootSignature.Reset(2, 0);
	m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"Camera Entity RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	step.PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));

	D3D12_INPUT_ELEMENT_DESC vertexElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetRasterizerState(premade::g_RasterizerBackSided);
	m_PSO.SetBlendState(premade::g_BlendNoColorWrite);
	m_PSO.SetDepthStencilState(premade::g_DepthStateDisabled);
	m_PSO.SetInputLayout(_countof(vertexElements), vertexElements);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE); // D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE is right.
	m_Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	m_PSO.SetRenderTargetFormats(0, nullptr, bufferManager::g_SceneDepthBuffer.GetFormat());
	m_PSO.SetVertexShader(g_pGeneralPurposeVS, sizeof(g_pGeneralPurposeVS));
	m_PSO.SetPixelShader(g_pGeneralPurposePS, sizeof(g_pGeneralPurposePS));
	m_PSO.Finalize();

	step.PushBack(std::make_shared<GraphicsPSO>(m_PSO));

	Float3Buffer colorBuffer;
	colorBuffer = DirectX::XMFLOAT3(0.2f, 0.2f, 0.6f);
	step.PushBack(std::make_shared<Float3Buffer>(colorBuffer));

	step.PushBack(std::make_shared<TransformBuffer>());

	DrawLine.PushBackStep(std::move(step));
	AddTechnique(std::move(DrawLine));
}