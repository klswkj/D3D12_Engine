#pragma once
#include "RenderQueuePass.h"
#include "RootSignature.h"
#include "PSO.h"

namespace custom
{
	class CommandContext;
	class GraphicsContext;
	class ComputeContext;
}

class PixelBuffer;
class DepthBuffer;

class OutlineDrawingPass final : public ID3D12RenderQueuePass
{
public:
	OutlineDrawingPass
	(
		std::string Name, 
		custom::RootSignature* pRootSignature = nullptr, 
		GraphicsPSO* pOutlineMaskPSO = nullptr, 
		GraphicsPSO* pOutlineDrawPSO = nullptr
	);
	~OutlineDrawingPass();

	void ExecutePass() DEBUG_EXCEPT final;
	void RenderWindow() final;

private:
	void ExecuteStencilMasking(custom::GraphicsContext& graphicsContext, DepthBuffer& TargetBuffer, uint8_t commandIndex);
	void ExecuteDrawColor(custom::GraphicsContext& graphicsContext, ColorBuffer& TargetBuffer, uint8_t commandIndex);
	void ExecuteBlurOutlineBuffer(custom::ComputeContext& computeContext, ColorBuffer& TargetBuffer, ColorBuffer& HelpBuffer, uint8_t commandIndex);
	void ExecutePaperOutline(custom::GraphicsContext& graphicsContext, ColorBuffer& TargetBuffer, DepthBuffer& DepthStencilBuffer, ColorBuffer& SrcBuffer, uint8_t commandIndex);

	void ExecuteHorizontalBlur(custom::GraphicsContext& graphicsContext, ColorBuffer& OutlineBuffer, ColorBuffer& HelpBuffer, uint8_t commandIndex);
	void ExecuteVerticalBlurAndPaper(custom::GraphicsContext& graphicsContext, ColorBuffer& TargetBuffer, DepthBuffer& DepthStencilBuffer, ColorBuffer& HelpBuffer, uint8_t commandIndex);

private:
	static OutlineDrawingPass* s_pOutlineDrawingPass;

private:
	custom::RootSignature* m_pRootSignature;
	GraphicsPSO*           m_pOutlineMaskPSO;
	GraphicsPSO*           m_pOutlineDrawPSO;

	custom::RootSignature m_BlurRootSignature;
	ComputePSO m_GaussBlurPSO;

	GraphicsPSO m_PaperOutlinePSO;
	GraphicsPSO m_HorizontalOutlineBlurPSO;
	GraphicsPSO m_VerticalOutlineBlurPSO;

	__declspec(align(16)) struct BlurConstants
	{
		float    BlurSigma;
		int32_t  BlurRadius;
		int32_t  BlurCount;
		float    BlurIntensity;
	} m_BlurConstants;
	
	__declspec(align(16)) struct BlurConstants2
	{
		float    BlurSigma;
		int32_t  BlurRadius;
		int32_t  BlurDirection; // If 0, is Vertical, else Horizontal.
		float    BlurIntensity;
	} m_BlurConstants2;
	
	bool m_bAllocateRootSignature  = false;
	bool m_bAllocateOutlineMaskPSO = false;
	bool m_bAllocateOutlineDrawPSO = false;
	bool m_bUseComputeShaderVersion = false;
};