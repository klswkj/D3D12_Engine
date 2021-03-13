#pragma once
#include "Matrix4.h"
#include "UAVBuffer.h"
#include "ColorBuffer.h"
#include "ShadowBuffer.h"
#include "PSO.h"
#include "RootSignature.h"
#include "Pass.h"

#include "ShaderConstantsTypeDefinitions.h"

namespace custom
{
	class CommandContext;
}

class LightPrePass : public D3D12Pass
{
	friend class MasterRenderGraph;
public:
	LightPrePass(std::string pName);
	~LightPrePass() {};
	void ExecutePass() DEBUG_EXCEPT final;
	void RenderWindow() DEBUG_EXCEPT final;
	void CreateLight(const uint32_t LightType);
	void CreateSphereLight();
	void CreateConeLight();
	void CreateConeShadowedLight();

	uint32_t GetFirstConeLightIndex();
	uint32_t GetFirstConeShadowedLightIndex();

private:
	void sortContainer(); // For LIGHT_GRID_PRELOADING
	void recreateBuffers();

private:
	static const char* sm_LightLabel[3];
	static uint32_t    sm_MaxLight;
	static const uint32_t sm_kShadowBufferSize;
	static const uint32_t sm_kMinWorkGroupSize;
	// todo: assumes natvie resolution of 1920x1080
	static const uint32_t sm_kLightGridCells;
	static const uint32_t sm_lightGridBitMaskSizeBytes;

private:
	size_t   m_DirtyLightIndex             = -1; 
	uint32_t m_FirstConeLightIndex         = -1;
	uint32_t m_FirstConeShadowedLightIndex = -1;
};