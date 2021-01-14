#include "stdafx.h"
#include "CameraEntity.h"
#include "Camera.h"
#include "PreMadePSO.h"
#include "BufferManager.h"
#include "RootSignature.h"
#include "PSO.h"
#include "UAVBuffer.h"

#include "Technique.h"
#include "MaterialConstants.h"

#include "ObjectFilterFlag.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#endif

#pragma push_macro("X")
#pragma push_macro("Y")
#pragma push_macro("Z")
#pragma push_macro("THALF")
#pragma push_macro("TSPACE")

#define X 1.0f
#define Y 0.75f
#define Z -2.0f
#define THALF 0.5f
#define TSPACE 0.15f

CameraEntity::CameraEntity(Camera* pCamera)
{
	// x = 1.0f; y = 0.75f; z  -2.0f;
	// tHalf = 0.5f; tSpace = 0.15f

	DirectX::XMFLOAT3 vertices[8] = { {-X,  Y, 0.0f},
			                          { X,  Y, 0.0f},
			                          { X,  Y, 0.0f},
			                          {-X, -Y, 0.0f},
			                          {0.0f, 0.0f, -Z},
	                                  {-THALF, Y + THALF, 0.0f},
	                                  { THALF, Y + THALF, 0.0f},
	                                  {0.0f, THALF + TSPACE, 0.0f} };
#pragma pop_macro("X")
#pragma pop_macro("Y")
#pragma pop_macro("Z")
#pragma pop_macro("THALF")
#pragma pop_macro("TSPACE")

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

	// m_VerticesBuffer.Create(L"Camera Entity VertexBuffer", _countof(vertices), sizeof(DirectX::XMFLOAT3), vertices);
	// m_IndicesBuffer.Create(L"Camera Entity IndexBuffer", _countof(indices), sizeof(uint8_t), indices);

	custom::StructuredBuffer* pVertexBuffer = 
		custom::StructuredBuffer::CreateVertexBuffer(L"Camera_Entity_VB", _countof(vertices), sizeof(DirectX::XMFLOAT3), vertices);

	custom::ByteAddressBuffer* pIndexBuffer =
		custom::ByteAddressBuffer::CreateIndexBuffer(L"Camera_Entity_IB", _countof(indices), sizeof(uint8_t), indices);

	D3D12_VERTEX_BUFFER_VIEW ModelVBV = pVertexBuffer->VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW ModelIBV = pIndexBuffer->IndexBufferView();

	m_IndexCount = _countof(indices);
	m_StartIndexLocation = 0;
	m_BaseVertexLocation = 0;
	m_VertexBufferView = pVertexBuffer->VertexBufferView();
	m_IndexBufferView = pIndexBuffer->IndexBufferView();

	ComputeBoundingBox((const float*)vertices, _countof(vertices), 3);

	Technique DrawLine{ "Draw Camera", eObjectFilterFlag::kOpaque };
	Step DrawLineInMainPass("MainRenderPass");

	m_RootSignature.Reset(2, 0);
	m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"CameraEntity_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// DrawLineInMainPas.PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));

	D3D12_INPUT_ELEMENT_DESC vertexElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	DXGI_FORMAT ColorFormat = bufferManager::g_SceneColorBuffer.GetFormat();
	DXGI_FORMAT DepthFormat = bufferManager::g_SceneDepthBuffer.GetFormat();

	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetRasterizerState(premade::g_RasterizerBackSided);
	m_PSO.SetBlendState(premade::g_BlendNoColorWrite);
	m_PSO.SetDepthStencilState(premade::g_DepthStateDisabled);
	m_PSO.SetInputLayout(_countof(vertexElements), vertexElements);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE); // D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE is right.
	m_Topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	m_PSO.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
	m_PSO.SetVertexShader(g_pFlat_VS, sizeof(g_pFlat_VS));
	m_PSO.SetPixelShader(g_pFlat_PS, sizeof(g_pFlat_PS));
	m_PSO.Finalize(L"CameraEntity_PSO");

	DrawLineInMainPass.PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));

	DrawLineInMainPass.PushBack(std::make_shared<GraphicsPSO>(m_PSO));

	Color3Buffer colorBuffer({ 0.2f, 0.2f, 0.6f });
	DrawLineInMainPass.PushBack(std::make_shared<Color3Buffer>(colorBuffer));

	DrawLineInMainPass.PushBack(std::make_shared<TransformBuffer>());

	DrawLine.PushBackStep(std::move(DrawLineInMainPass));
	AddTechnique(std::move(DrawLine));
	// DrawLine.PushBackStep(std::move(DrawLineInMainPass));
	// 

	// AddTechnique(std::move(DrawLine));

	m_pParentCamera = pCamera;
}

/*
Math::Matrix4 CameraEntity::GetTransform() const noexcept
{
	return Math::Matrix4(DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMVECTOR(m_Rotation)) *
		DirectX::XMMatrixTranslationFromVector(DirectX::XMVECTOR(m_CameraPosition)));
}
*/
Math::Matrix4 CameraEntity::GetTransform() const noexcept
{
	return Math::Matrix4(m_pParentCamera->GetRightUpForwardMatrix(), m_pParentCamera->GetPosition());
}