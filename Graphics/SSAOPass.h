#pragma once
#include "Pass.h"
#include "RootSignature.h"
#include "PSO.h"
#include "Pass.h"

namespace custom
{
	class CommandContext;
	class ComputeContext;
}

class IBaseCamera;
class Camera;
class ColorBuffer;

// TODO : Camera* m_pCurrentCamera 만들어서 카메라 받고 렌더링하기
class SSAOPass : public Pass
{
	friend class MasterRenderGraph;
	friend class MainRenderPass;
public:
	SSAOPass(std::string pName);
	~SSAOPass() {}
	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;
	void ComputeAO(custom::ComputeContext& BaseContext, ColorBuffer& Result, ColorBuffer& _DepthBuffer, const float TangentHalfFOVHorizontal);
	void BlurAndUpsampling
	(
		custom::ComputeContext& Context, 
		ColorBuffer& Result, ColorBuffer& HighResolutionDepth, ColorBuffer& LowResolutionDepth,
		ColorBuffer* InterleavedAO, ColorBuffer* HighQualityAO, ColorBuffer* HighResolutionAO
	);
	void RenderWindow();
public:
	bool m_bEnable;
	bool m_bDebugDraw;     // When DebugDraw is Enable, after all of passes' draw will be disabled.
	bool m_bAsyncCompute;
	bool m_bComputeLinearZ;
private:
	static SSAOPass* s_pSSAOPass;

	float m_NoiseFilterTolerance; // or be named Threhold
	float m_BlurTolerance;
	float m_UpsampleTolerance;
	float m_RejectionFallOff;
	float m_Accentuation;
	float m_ScreenSpaceDiameter;
	int m_HierarchyDepth;

	enum class QualityLevel { kVeryLow = 0, kLow, kMedium, kHigh, kVeryHigh, kCount };
	const char* m_QualityLabel[5] = { "VeryLow", "Low", "Medium", "High", "VeryHigh" };
	QualityLevel m_eQuality;

	custom::RootSignature m_MainRootSignature;
	custom::RootSignature m_LinearizeDepthSignature;

	ComputePSO m_DepthPrepareCS_16;
	ComputePSO m_DepthPrepareCS_64;
	ComputePSO m_RenderWithWideSamplingCS;
	ComputePSO m_RenderWithDepthArrayCS; 

	ComputePSO m_BlurUpSampleBlendWithHighResolutionCS;
	ComputePSO m_BlurUpSampleBlendWithBothCS;
	ComputePSO m_BlurUpSampleFinalWithNoneCS;
	ComputePSO m_BlurUpSampleFinalWithCombineLowerResolutionCS;
	
	ComputePSO m_LinearizeDepthCS;
	ComputePSO m_DebugSSAOCS;

	float m_SampleThickness[12];
};