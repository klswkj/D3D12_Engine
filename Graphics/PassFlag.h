#pragma once

enum class ePass
{
	PrePass = 0,
	MainPass,
	PostEffectPass,
	Count
};

enum class ePrePass
{
	LightingPrePass = 0,
	ShadowPrePass,
	Z_PrePass,
	Count
};

enum class eMainPass
{
	SSAOPass = 0,
	FillLightGridPass,
	ShadowMappingPass,
	MainRenderPass,
	Count
};

enum class ePostEffectPass
{
	TA_Pass = 0,
	DOFPass,
	Count
};