#pragma once
#include "stdafx.h"
#include "Pass.h"
#include "UpdateConstantBuffer.h"

#include "RootSignature.h"
#include "PSO.h"

class DOFPass : public Pass
{
public:
	DOFPass(std::string name);
	~DOFPass();

	void Execute(custom::CommandContext& BaseContext, float NearClipDist, float FarClipDist) DEBUG_EXCEPT;
	void RenderSubWindow();

	// ���⼭ SpawnControlWidgets���� Imgui Rendering�ϴ°� ���ٴ�, 
	// ��� Pass�� ��Ƽ� �Ѳ����� �����ϴ� â�� ����°� �� ���� �� ����.
	// ���� ���� ������� Ǯ���, �� �����̳ʿ� ��� �н��� ����ְ�, 
	// Pass�����ͷθ� SpawnControlWidgets()�� ȣ���ϴ� �������.

private:
	custom::IndirectArgsBuffer m_IndirectArgument;
	custom::RootSignature m_RootSignature;

	ComputePSO DoFPass1CS;                  // Responsible for classifying tiles (1st pass)
	ComputePSO DoFTilePassCS;               // Disperses tile info to its neighbors (3x3)
	ComputePSO DoFTilePassFixupCS;          // Searches for straggler tiles to "fixup"
	ComputePSO DoFPreFilterCS;              // Full pre-filter with variable focus
	ComputePSO DoFPreFilterFastCS;          // Pre-filter assuming near-constant focus
	ComputePSO DoFPreFilterFixupCS;         // Pass through colors for completely in focus tile
	ComputePSO DoFPass2CS;                  // Perform full CoC convolution pass
	ComputePSO DoFPass2FastCS;              // Perform color-only convolution for near-constant focus
	ComputePSO DoFPass2FixupCS;             // Pass through colors again
	ComputePSO DoFMedianFilterCS;           // 3x3 median filter to reduce fireflies
	ComputePSO DoFMedianFilterSepAlphaCS;   // 3x3 median filter to reduce fireflies (separate filter on alpha)
	ComputePSO DoFMedianFilterFixupCS;      // Pass through without performing median
	ComputePSO DoFCombineCS;                // Combine DoF blurred buffer with focused color buffer
	ComputePSO DoFCombineFastCS;            // Upsample DoF blurred buffer

	uint32_t m_bEnable          = true;
	uint32_t m_bEnablePreFilter = true;
	uint32_t m_bMedianFilter    = true;
	uint32_t m_bMedianAlpha     = true;
	uint32_t m_bDebugMode       = true;
	uint32_t m_bDebugTile       = true;

	float m_FocalDepth        = 0.1f;
	float m_FocalRange        = 0.1f;
	float m_ForegroundRange   = 100.0f;
	float m_AntiSparkleWeight = 100.0f;

	__declspec(align(16)) struct DoFConstantBuffer
	{
		float FocalCenter, FocalSpread;
		float FocalMinZ, FocalMaxZ;
		float RcpBufferWidth, RcpBufferHeight;
		uint32_t BufferWidth, BufferHeight;
		int32_t HalfWidth, HalfHeight;
		uint32_t TiledWidth, TiledHeight;
		float RcpTiledWidth, RcpTiledHeight;
		uint32_t DebugState, PreFilterState;
		float FGRange, RcpFGRange, AntiSparkleFilterStrength;
	};

	DoFConstantBuffer cBuffer;
};