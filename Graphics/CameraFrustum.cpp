#include "CameraFrustum.h"
#include "PremadePSO.h"
#include "BufferManager.h"
#include "RootSignature.h"
#include "PSO.h"

#include "Technique.h"
#include "Step.h"
#include "VariableConstantBuffer.h"

#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GeneralPurposePS.h"   // Pseudo Shader
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/GeneralPurposeVS.h"
#elif !defined(_DEBUG) | defined(NDEBUG)
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/GeneralPurposePS.h" // Pseudo Shader
#include "../x64/RELEASE/Graphics(.lib)/CompiledShaders/GeneralPurposeVS.h"
#endif
/*
OrthogonalTransform
1. Quaternion ( Rotation )
2. Vector3    ( Translation )

AffineTransform
1. Basis     ( Matrix3 )
2. Vector3   ( Translation )
*/

CameraFrustum::CameraFrustum(float AspectHeightOverWidth, float NearZ, float FarZ)
{
	// Vertices : 8, Indices : 24

	SetVertices(AspectHeightOverWidth, NearZ, FarZ);

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

	m_IndicesBuffer.Create(L"Camera Frustum Index Buffer", _countof(Indices), sizeof(uint8_t), &Indices[0]);

	Technique line{ "Draw Frustum", eObjectFilterFlag::kOpaque };

	{
		m_RootSignature.Reset(2, 0);
		m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
		m_RootSignature[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSignature.Finalize(L"Camera Frustum RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}
	{
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
	}
	{
		Step occuluded("Lambertian");

		occuluded.PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));
		occuluded.PushBack(std::make_shared<GraphicsPSO>(m_PSO));

		Float3Buffer colorBuffer;
		colorBuffer = DirectX::XMFLOAT3(0.2f, 0.2f, 0.6f);
		occuluded.PushBack(std::make_shared<Float3Buffer>(colorBuffer));

		occuluded.PushBack(std::make_shared<TransformBuffer>());

		line.PushBackStep(std::move(occuluded));
	}
	{
		Step occuluded("WireFrame");

		occuluded.PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));
		occuluded.PushBack(std::make_shared<GraphicsPSO>(m_PSO));

		Float3Buffer colorBuffer;
		colorBuffer = DirectX::XMFLOAT3(0.25f, 0.08f, 0.08f);
		occuluded.PushBack(std::make_shared<Float3Buffer>(colorBuffer));

		occuluded.PushBack(std::make_shared<TransformBuffer>());

		line.PushBackStep(std::move(occuluded));
	}
	// techniques.push_back(std::move(line));
	AddTechnique(std::move(line));
}
//         UpVector axis           
// float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip
void CameraFrustum::SetVertices(float Width, float Height, float NearZClip, float FarZClip)
{
	float aspectHeightOverWidth = 0;

	const float nearZCipWidth = aspectHeightOverWidth * NearZClip * 0.5f;

	const float zRatio          = FarZClip / NearZClip;
	const float NearZClipWidth  = Width  / 2.0f;
	const float NearZClipHeight = Height / 2.0f;
	const float FarZClipWidth   = NearZClipWidth * zRatio;
	const float FarZClipHeight  = NearZClipHeight * zRatio;

	DirectX::XMFLOAT3 vertices[8] =
	{
		{-NearZClipWidth,  NearZClipHeight, NearZClip },
		{ NearZClipWidth,  NearZClipHeight, NearZClip },
		{ NearZClipWidth, -NearZClipHeight, NearZClip },
		{-NearZClipWidth, -NearZClipHeight, NearZClip },
		{ -FarZClipWidth,   FarZClipHeight,  FarZClip },
		{  FarZClipWidth,   FarZClipHeight,  FarZClip },
		{  FarZClipWidth,  -FarZClipHeight,  FarZClip },
		{ -FarZClipWidth,  -FarZClipHeight,  FarZClip }
	};

	m_VerticesBuffer.Create(L"Camera Frustum Vertex Buffer", _countof(vertices), sizeof(DirectX::XMFLOAT3), &vertices[0]);
}

void CameraFrustum::SetVertices(float AspectHeightOverWidth, float NearZClip, float FarZClip)
{
	const float zRatio = FarZClip / NearZClip;
	const float NearZClipWidth = NearZClip * 0.5f;
	const float NearZClipHeight = (1 / AspectHeightOverWidth) * NearZClip * 0.5f;
	const float FarZClipWidth = NearZClipWidth * zRatio;
	const float FarZClipHeight = NearZClipHeight * zRatio;

	DirectX::XMFLOAT3 vertices[8] =
	{
		{-NearZClipWidth,  NearZClipHeight, NearZClip },
		{ NearZClipWidth,  NearZClipHeight, NearZClip },
		{ NearZClipWidth, -NearZClipHeight, NearZClip },
		{-NearZClipWidth, -NearZClipHeight, NearZClip },
		{ -FarZClipWidth,   FarZClipHeight,  FarZClip },
		{  FarZClipWidth,   FarZClipHeight,  FarZClip },
		{  FarZClipWidth,  -FarZClipHeight,  FarZClip },
		{ -FarZClipWidth,  -FarZClipHeight,  FarZClip }
	};

	m_VerticesBuffer.Create(L"Camera Frustum Vertex Buffer", _countof(vertices), sizeof(DirectX::XMFLOAT3), &vertices[0]);
}