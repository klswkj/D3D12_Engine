#pragma once
#include "Matrix4.h"
#include "UAVBuffer.h"
#include "ColorBuffer.h"
#include "ShadowBuffer.h"
#include "PSO.h"
#include "RootSignature.h"

namespace custom
{
	class CommandContext;
}

class LightPrePass
{
	friend class MasterRenderGraph;
public:
	LightPrePass();
	~LightPrePass();
	void RenderWindow();
	void CreateSphereLight();
	void CreateConeLight();

private:
	void sortContainers();
	void recreateBuffers();
private:
	const uint32_t m_kShadowBufferSize{ 512u };
	size_t m_DirtyLightIndex = -1;

	static uint32_t sm_MaxLight;
	static const char* sm_LightLabel[3];

	// must keep in sync with HLSL
	
	uint32_t m_FirstConeLightIndex = -1;
	uint32_t m_FirstConeShadowedLightIndex = -1;

	struct CSConstants
	{
		uint32_t ViewportWidth, ViewportHeight;
		float InvTileDim;
		float RcpZMagic;
		uint32_t TileCount;
		Math::Matrix4 ViewProjMatrix;
		// float ViewPorjMatrx[16];
	};
};

// float3 pos;
// float radiusSq;
// float3 color;
// uint type;
// float3 coneDir;
// float2 coneAngles; // x = 1.0f / (cos(coneInner) - cos(coneOuter)), y = cos(coneOuter)
// float4x4 shadowTextureMatrix;
/*
inline ShadowBuffer& LightPrePass::GetShadowBuffer()
{
	return m_LightShadowTempBuffer;
}

inline size_t LightPrePass::GetLightSize()
{
	size_t LightDataSize = m_Lights.size();
	size_t LightShadowMatrixesSize = m_LightShadowMatrixes.size();
	ASSERT(LightDataSize == LightShadowMatrixesSize, "Light and ShadowMatrix containers' sizes are not the same.");

	return LightDataSize;
}
*/