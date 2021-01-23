#pragma once
#include "RenderQueuePass.h"
#include "RootSignature.h"
#include "PSO.h"

namespace custom
{
	class CommandContext;
}

class PixelBuffer;
class DepthBuffer;

class OutlineDrawingPass : public RenderQueuePass
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

	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void RenderWindow() override;
private:
	void ExecuteStencilMasking(custom::CommandContext& BaseContext, DepthBuffer& TargetBuffer);
	void ExecuteDrawColor(custom::CommandContext& BaseContext, ColorBuffer& TargetBuffer);
	void ExecuteBlurOutlineBuffer(custom::CommandContext& BaseContext, ColorBuffer& TargetBuffer, ColorBuffer& HelpBuffer);
	void ExecutePaperOutline(custom::CommandContext& BaseContext, ColorBuffer& TargetBuffer, DepthBuffer& DepthStencilBuffer, ColorBuffer& SrcBuffer);

	void ExecuteHorizontalBlur(custom::CommandContext& BaseContext, ColorBuffer& OutlineBuffer, ColorBuffer& HelpBuffer);
	void ExecuteVerticalBlurAndPaper(custom::CommandContext& BaseContext, ColorBuffer& TargetBuffer, DepthBuffer& DepthStencilBuffer, ColorBuffer& HelpBuffer);

private:
	static OutlineDrawingPass* s_pOutlineDrawingPass;

	custom::RootSignature* m_pRootSignature;
	GraphicsPSO* m_pOutlineMaskPSO;
	GraphicsPSO* m_pOutlineDrawPSO;

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