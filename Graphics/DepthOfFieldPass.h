#pragma once
#include "stdafx.h"
#include "Pass.h"
#include "UpdateConstantBuffer.h"

#include "UAVBuffer.h"
#include "RootSignature.h"
#include "PSO.h"

class DOFPass : public Pass
{
public:
	DOFPass(std::string name);
	~DOFPass();

	void Execute(custom::CommandContext& BaseContext, float NearClipDist, float FarClipDist) DEBUG_EXCEPT;
	void RenderSubWindow();

	// 여기서 SpawnControlWidgets으로 Imgui Rendering하는것 보다는, 
	// 모든 Pass를 모아서 한꺼번에 관리하는 창을 만드는게 더 좋을 거 같다.
	// 위의 같은 방식으로 풀어가고, 한 컨테이너에 모든 패스를 집어넣고, 
	// Pass포인터로만 SpawnControlWidgets()를 호출하는 방식으로.

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

	uint32_t m_bEnable{ true };
	uint32_t m_bEnablePreFilter{ true };
	uint32_t m_bMedianFilter{ true };
	uint32_t m_bMedianAlpha{ true };
	uint32_t m_bDebugMode{ true };
	uint32_t m_bDebugTile{ true };

	float m_FocalDepth{ 0.1f };
	float m_FocalRange{ 0.1f };
	float m_ForegroundRange{ 100.0f };
	float m_AntiSparkleWeight{ 100.0f };

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
	// uint32_t m_bDebugMode{ true };
	// uint32_t m_bDebugTiles{ true };

	// float focalDepth[4]{ 0.1f, 0.0f, 1.0f, 0.01f }; // value, minValue, maxValue, stepSize
	// float focalRange[4]{ 0.1f, 0.0f, 1.0f, 0.01f };
	// float foregroundRange[4]{ 0.1f, 0.0f, 1.0f, 0.01f };
	// float antiSparkleWeight[4]{ 1.0f, 0.0f, 10.0f, 1.0f };

	///////////////////////↓ Convert to /////////////////////////

	// std::shared_ptr<CachingPixelConstantBufferEx> bEnable;           // { true }
	// std::shared_ptr<CachingPixelConstantBufferEx> bEnablePreFilter;  // { true }
	// std::shared_ptr<CachingPixelConstantBufferEx> bMedianFilter;     // { true }
	// std::shared_ptr<CachingPixelConstantBufferEx> bMedianAlpha;      // { true }
	// std::shared_ptr<CachingPixelConstantBufferEx> NearClipDist;      // 
	// std::shared_ptr<CachingPixelConstantBufferEx> FarClipDist;       // 
	// std::shared_ptr<CachingPixelConstantBufferEx> FocalDepth;        // { 0.1f, 0.0f, 1.0f, 0.01f } 
	// std::shared_ptr<CachingPixelConstantBufferEx> FocalRange;        // { 0.1f, 0.0f, 1.0f, 0.01f }
	// std::shared_ptr<CachingPixelConstantBufferEx> ForegroundRange;   // { 0.1f, 0.0f, 1.0f, 0.01f }
	// std::shared_ptr<CachingPixelConstantBufferEx> AntiSparkleWeight; // { 1.0f, 0.0f, 10.0f, 1.0f }
    // std::shared_ptr<CachingPixelConstantBufferEx> DOFCBuffer;
};

/*
	DoFConstantBuffer cbuffer =
	{
		(float)FocalDepth, 1.0f / (float)FocalRange,
		(float)FocalDepth - (float)FocalRange, (float)FocalDepth + (float)FocalRange,
		1.0f / BufferWidth, 1.0f / BufferHeight,
		BufferWidth, BufferHeight,
		(int32_t)HashInternal::DivideByMultiple(BufferWidth, 2), (int32_t)HashInternal::DivideByMultiple(BufferHeight, 2),
		TiledWidth, TiledHeight,
		1.0f / TiledWidth, 1.0f / TiledHeight,
		(uint32_t)DebugMode, EnablePreFilter ? 0u : 1u,
		ForegroundRange / FarClipDist, FarClipDist / ForegroundRange, (float)AntiSparkleWeight
	};
	*/