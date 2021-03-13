#include "stdafx.h"
#include "CameraFrustum.h"
#include "PremadePSO.h"
#include "BufferManager.h"

#include "IBaseCamera.h"
#include "Camera.h"
#include "Technique.h"
#include "Step.h"
#include "MaterialConstants.h"
#include "ColorConstants.h"
#include "TransformConstants.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#endif

#define TransformRootIndex 1u
#define Color3RootIndex    3u

CameraFrustum::CameraFrustum(Camera& rCamera, const float aspectHeightOverWidth, const float nearZ, const float farZ)
	:
	m_pParentCamera(&rCamera),
	m_CameraPosition({ 0.0f,0.0f,0.0f }),
	m_Rotation({ 0.0f,0.0f,0.0f })
{
	// Vertices : 8, Indices : 24

	SetVertices(aspectHeightOverWidth, nearZ, farZ);

	uint8_t Indices[24] =
	{
		0, 1,
		1, 2,
		2, 3,
		3, 0,
		4, 5,
		5, 6,
		6, 7,
		7, 4,
		0, 4,
		1, 5,
		2, 6,
		3, 7
	};

	// m_IndicesBuffer.Create(L"Camera Frustum Index Buffer", _countof(Indices), sizeof(uint8_t), &Indices[0]);

	custom::ByteAddressBuffer* pIndexBuffer =
		custom::ByteAddressBuffer::CreateIndexBuffer(L"Camera_Frustum_IB", _countof(Indices), sizeof(uint8_t), Indices);

	D3D12_INDEX_BUFFER_VIEW ModelIBV = pIndexBuffer->IndexBufferView();

	m_IndexCount = _countof(Indices);
	m_StartIndexLocation = 0U;
	m_BaseVertexLocation = 0U;
	// m_VertexBufferView = pVertexBuffer->VertexBufferView();
	m_IndexBufferView = pIndexBuffer->IndexBufferView();

	Technique DrawFrustumTechnique{ "Draw Frustum", eObjectFilterFlag::kOpaque };

	{
		m_RootSignature.Reset(3, 0);
		m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
		m_RootSignature[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_VERTEX);
		m_RootSignature[2].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSignature.Finalize(L"CameraFrustum_RS", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
		m_PSO.SetRenderTargetFormats(1, &ColorFormat, bufferManager::g_SceneDepthBuffer.GetFormat());
		m_PSO.SetVertexShader(g_pFlat_VS, sizeof(g_pFlat_VS));
		m_PSO.SetPixelShader(g_pFlat_PS, sizeof(g_pFlat_PS));
		m_PSO.Finalize(L"CameraFrustum_PSO");
	}
	{
		Step DrawLineInMainPass("MainRenderPass");

		DrawLineInMainPass.PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));
		DrawLineInMainPass.PushBack(std::make_shared<GraphicsPSO>(m_PSO));

		// Color3Buffer colorBuffer({ 0.2f, 0.2f, 0.6f });
		DrawLineInMainPass.PushBack(std::make_shared<Color3Buffer>(Color3RootIndex));

		DrawLineInMainPass.PushBack(std::make_shared<TransformConstants>(TransformRootIndex));

		DrawFrustumTechnique.PushBackStep(std::move(DrawLineInMainPass));
	}
	{
		/*
		Step occuluded("wireframe");

		occuluded.PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));
		occuluded.PushBack(std::make_shared<GraphicsPSO>(m_PSO));

		Color3Buffer colorBuffer({0.25f, 0.08f, 0.08f});

		occuluded.PushBack(std::make_shared<Color3Buffer>(colorBuffer));
		occuluded.PushBack(std::make_shared<TransformBuffer>());

		DrawFrustumTechnique.PushBackStep(std::move(occuluded));
		*/
	}
	// m_Techniques.push_back(std::move(DrawFrustumTechnique));
	AddTechnique(std::move(DrawFrustumTechnique));
}
//         UpVector axis           
// float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip
void CameraFrustum::SetVertices(const float width, const float height, const float nearZClip, const float farZClip)
{
	float AspectHeightOverWidth = 0;

	const float NearZCipWidth = AspectHeightOverWidth * nearZClip * 0.5f;

	const float ZRatio          = farZClip / nearZClip;
	const float NearZClipWidth  = width  / 2.0f;
	const float NearZClipHeight = height / 2.0f;
	const float FarZClipWidth   = NearZClipWidth * ZRatio;
	const float FarZClipHeight  = NearZClipHeight * ZRatio;

	DirectX::XMFLOAT3 vertices[8] =
	{
		{-NearZClipWidth,  NearZClipHeight, nearZClip },
		{ NearZClipWidth,  NearZClipHeight, nearZClip },
		{ NearZClipWidth, -NearZClipHeight, nearZClip },
		{-NearZClipWidth, -NearZClipHeight, nearZClip },
		{ -FarZClipWidth,   FarZClipHeight,  farZClip },
		{  FarZClipWidth,   FarZClipHeight,  farZClip },
		{  FarZClipWidth,  -FarZClipHeight,  farZClip },
		{ -FarZClipWidth,  -FarZClipHeight,  farZClip }
	};

	custom::StructuredBuffer* pVertexBuffer =
		custom::StructuredBuffer::CreateVertexBuffer(L"Camera_Frustum_VB1", _countof(vertices), sizeof(DirectX::XMFLOAT3), vertices);

	m_VertexBufferView = pVertexBuffer->VertexBufferView();
	ComputeBoundingBox((const float*)vertices, _countof(vertices), 3);
}

void CameraFrustum::SetVertices(const float aspectHeightOverWidth, const float nearZClip, const float farZClip)
{
	const float ZRatio = farZClip / nearZClip;
	const float NearZClipWidth = nearZClip * 0.5f;
	const float NearZClipHeight = (1 / aspectHeightOverWidth) * nearZClip * 0.5f;
	const float FarZClipWidth = NearZClipWidth * ZRatio;
	const float FarZClipHeight = NearZClipHeight * ZRatio;

	DirectX::XMFLOAT3 vertices[8] =
	{
		{-NearZClipWidth,  NearZClipHeight, nearZClip },
		{ NearZClipWidth,  NearZClipHeight, nearZClip },
		{ NearZClipWidth, -NearZClipHeight, nearZClip },
		{-NearZClipWidth, -NearZClipHeight, nearZClip },
		{ -FarZClipWidth,   FarZClipHeight,  farZClip },
		{  FarZClipWidth,   FarZClipHeight,  farZClip },
		{  FarZClipWidth,  -FarZClipHeight,  farZClip },
		{ -FarZClipWidth,  -FarZClipHeight,  farZClip }
	};

	custom::StructuredBuffer* pVertexBuffer =
		custom::StructuredBuffer::CreateVertexBuffer(L"Camera_Frustum_VB2", _countof(vertices), sizeof(DirectX::XMFLOAT3), vertices);

	m_VertexBufferView = pVertexBuffer->VertexBufferView();
	ComputeBoundingBox((const float*)vertices, _countof(vertices), 3);
}

void CameraFrustum::SetPosition(const DirectX::XMFLOAT3& position) noexcept
{
	m_CameraPosition = position;
	// m_CameraToWorld.SetTranslation(Position);
}

void CameraFrustum::SetPosition(const Math::Vector3& translation) noexcept
{
	m_CameraPosition = translation;
	// m_CameraToWorld.SetTranslation(Translation);
}

void CameraFrustum::SetRotation(const DirectX::XMFLOAT3& rollPitchYaw) noexcept
{
	m_Rotation = rollPitchYaw;
	// m_CameraToWorld.SetRotation(RollPitchYaw);
}

void CameraFrustum::SetRotation(const Math::Vector3& rotation) noexcept
{
	m_Rotation = rotation;
}

Math::Matrix4 CameraFrustum::GetTransform() const noexcept
{
	return Math::Matrix4(m_pParentCamera->GetRightUpForwardMatrix(), m_pParentCamera->GetPosition());
}