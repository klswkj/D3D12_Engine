#include "stdafx.h"
#include "OutlineDrawingPass.h"
#include "CommandContext.h"
#include "BindingPass.h"
#include "RenderQueuePass.h"
#include "RenderTarget.h"
#include "BufferManager.h"
#include "PremadePSO.h"


#if defined(_DEBUG) | !defined(NDEBUG)
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Debug/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#elif !defined(_DEBUG) | defined(RELEASE)
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_VS.h"
#include "../x64/Release/Graphics(.lib)/CompiledShaders/Flat_PS.h"
#endif


OutlineDrawingPass::OutlineDrawingPass(std::string pName, UINT BufferWidth, UINT BufferHeight)
	: RenderQueuePass(pName)
{
	m_RootSignature.Reset(2, 0);
	m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize
	(
		L"OutlineDrawingPass", 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
	);

	m_GraphicsPSO.SetInputLayout(5, premade::g_InputElements); // 3 or 5
	m_GraphicsPSO.SetBlendState(premade::g_BlendDisable);
	m_GraphicsPSO.SetDepthStencilState(premade::g_StencilStateWriteOnly);
	m_GraphicsPSO.SetRasterizerState(premade::g_RasterizerDefault);
	m_GraphicsPSO.SetRootSignature(m_RootSignature);
	m_GraphicsPSO.SetVertexShader(g_pFlat_VS, _countof(g_pFlat_VS));
	m_GraphicsPSO.SetPixelShader(g_pFlat_PS, _countof(g_pFlat_PS));
	m_GraphicsPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_GraphicsPSO.SetRenderTargetFormats(0, nullptr, bufferManager::g_StencilBuffer.GetFormat());
	m_GraphicsPSO.Finalize();
	// 여기서 만든걸 BlurPass에 패스.

	PushBack(std::make_shared<custom::RootSignature>(m_RootSignature));
	PushBack(std::make_shared<GraphicsPSO>(m_GraphicsPSO));

	m_pRenderTarget = std::make_shared<ShaderInputRenderTarget>(bufferManager::g_StencilBuffer, m_RootSignature, m_GraphicsPSO, 3, 0);
}

void OutlineDrawingPass::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	m_pRenderTarget->Clear(BaseContext);
	RenderQueuePass::Execute(BaseContext);
}