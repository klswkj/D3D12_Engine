#pragma once
#include "RenderingResource.h"

class Entity;

#define DEFAULT_DIFFUSE_COLOR  Math::Vector3(0.18f, 0.18f, 0.18f)
#define DEFAULT_SPECULAR_COLOR Math::Vector3(0.45f, 0.45f, 0.45f)
#define DEFAULT_SPECULAR_GLOSS 128.0f
#define DEFAULT_WEIGHT         1.0f

// Keep Sync in HLSL.

class ControllerConstants0 : public RenderingResource
{
public:
	ControllerConstants0
	(
		Math::Vector3 DiffuseColor = DEFAULT_DIFFUSE_COLOR,
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR,
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS,
		float SpecularWeight = 1.0f
	);
	ControllerConstants0(ControllerConstants0&& Other);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void SetDiffuseColor(Math::Vector3 DiffuseColor);
	void SetDiffuseColorByElement(float R, float G, float B);
	void SetSpecularGloss(float SpecularGloss);

	void SetSpecularColor(Math::Vector3 SpecularColor);
	void SetSpecularColorByElement(float R, float G, float B);
	void SetSpecularWeight(float SpecularWeight);

	Math::Vector4 DiffuseColorAndSpecularGloss;
	Math::Vector4 SpecularColorAndWeight;
};

class ControllerConstants1 : public RenderingResource
{
public:
	ControllerConstants1
	(
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR, 
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		float SpecularWeight = 1.0f
	);
	ControllerConstants1(ControllerConstants1&& Other);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void SetSpecularColor(Math::Vector3 SpecularColor);
	void SetSpecularColorByElement(float R, float G, float B);
	void SetSpecularWeight(float SpecularWeight);

	void SetSpecularGloss(float SpecularGloss);

	Math::Vector4 SpecularColorAndWeight;
	float SpecularGloss;
};

// Specular
class ControllerConstants2 : public RenderingResource
{
public:
	ControllerConstants2
	(
		Math::Vector3 DiffuseColor = DEFAULT_DIFFUSE_COLOR,
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR, 
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		float SpecularWeight = 1.0f, 
		bool bUseSpecularMap = true, 
		bool bUseGlossAlpha = true
	);
	ControllerConstants2(ControllerConstants2&& Other);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	// Data Handle Method
	void SetDiffuseColor(Math::Vector3 DiffuseColor);
	void SetDiffuseColorByElement(float R, float G, float B);
	void SetUseGlossAlpha(bool bUseGlossAlpha);

	void SetUseSpecularMap(bool bUseSpecularMap);
	void SetSpecularWeight(float SpecularWeight);
	void SetSpecularGloss(float SpecularGloss);

	void SetSpecularColor(Math::Vector3 SpecularColor);
	void SetSpecularColorByElement(float R, float G, float B);

	Math::Vector4 DiffuseColorAndUseGlossAlpha;
	Math::Vector3 UseSpecularMapSpecularWeightGloss;
	DirectX::XMFLOAT3 SpecularColor;
};

// Diffuse Specular
class ControllerConstants3 : public RenderingResource
{
public:
	ControllerConstants3
	(
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR, 
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		float SpecularWeight = 1.0f,  
		bool bUseSpecularMap = true, 
		bool bUseGlossAlpha = true
	);
	ControllerConstants3(ControllerConstants3&& Other);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void SetUseGlossAlpha(bool bUseGlossAlpha);
	void SetUseSpecularMap(bool bUseSpecularMap);
	void SetSpecularWeight(float SpecularWeight);
	void SetSpecularGloss(float SpecularGloss);

	void SetSpecularColor(Math::Vector3 SpecularColor);
	void SetSpecularColorByElement(float R, float G, float B);

	Math::Vector4 UseGlossAlphaUseSpecularMapAndSpecularWeightGloss;
	DirectX::XMFLOAT3 SpecularColor;
};

// Normal
class ControllerConstants4 : public RenderingResource
{
public:
	ControllerConstants4
	(
		Math::Vector3 DiffuseColor = DEFAULT_DIFFUSE_COLOR, 
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR,  
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		float SpecularWeight = 1.0f, 
		float NormalMapWeight = 1.0f, 
		bool bUseNormalMap = true
	);
	ControllerConstants4(ControllerConstants4&& Other);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	// Data Handle Method
	void SetDiffuseColor(Math::Vector3 DiffuseColor);
	void SetDiffuseColorByElement(float R, float G, float B);
	void SetSpecularWeight(float SpecularWeight);

	void SetSpecularColor(Math::Vector3 SpecularColor);
	void SetSpecularColorByElement(float R, float G, float B);
	void SetSpecularGloss(float SpecularGloss);

	void SetUseNormalMap(bool bUseNormalMap);
	void SetNormalMapWeight(float NormalMapWeight);

	Math::Vector4 DiffuseColorAndSpecularWeight;
	Math::Vector4 SpecularColorGloss;
	DirectX::XMFLOAT2 UseNormalMapWeight;
};

// Diffuse Normal
class ControllerConstants5 : public RenderingResource
{
public:
	ControllerConstants5
	(
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR, 
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		float SpecularWeight = 1.0f, 
		float NormalMapWeight = 1.0f, 
		bool bUseNormalMap = true
	);
	ControllerConstants5(ControllerConstants5&& Other);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void SetSpecularWeight(float SpecularWeight);
	void SetSpecularGloss(float SpecularGloss);
	void SetUseNormalMap(bool bUseNormalMap);
	void SetNormalMapWeight(float NormalMapWeight);

	void SetSpecularColor(Math::Vector3 SpecularColor);
	void SetSpecularColorByElement(float R, float G, float B);

	Math::Vector4 SpecularWeightGlossUseNormalMapNormalWeight;
	DirectX::XMFLOAT3 SpecularColor;
};

// Specular Normal
class ControllerConstants6 : public RenderingResource
{
public:
	ControllerConstants6
	(
		Math::Vector3 DiffuseColor = DEFAULT_DIFFUSE_COLOR, 
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR, 
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		float SpecularWeight = 1.0f, 
		float NormalMapWeight = 1.0f, 
		bool bUseGlossAlpha = true, 
		bool bUseSpecularMap = true, 
		bool bUseNormalMap = true
	);
	ControllerConstants6(ControllerConstants6&& Other);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void SetDiffuseColor(Math::Vector3 DiffuseColor);
	void SetDiffuseColorByElement(float R, float G, float B);
	void SetUseGlossAlpha(bool bUseGlossAlpha);

	void SetUseSpecularMap(bool bUseSpecularMap);
	void SetSpecularWeight(float SpecularWeight);
	void SetSpecularGloss(float SpecularGloss);
	void SetUseNormalMap(bool bUseNormalMap);

	void SetSpecularColor(Math::Vector3 SpecularColor);
	void SetSpecularColorByElement(float R, float G, float B);
	void SetNormalMapWeight(float NormalMapWeight);

	Math::Vector4 DiffuseColorUseGlossAlpha;
	Math::Vector4 UseSpecularMapWeightGlossAndNormalMap;
	Math::Vector4 SpecularColorAndNormalMapWeight;
};

// Diffuse Specular Normal
class ControllerConstants7 : public RenderingResource
{
public:
	ControllerConstants7
	(
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR, 
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		float SpecularWeight = 1.0f, 
		float NormalMapWeight = 1.0f, 
		bool bUseGlossAlpha = true, 
		bool bUseSpecularMap = true,
		bool bUseNormalMap = true
	);
	ControllerConstants7(ControllerConstants7&& Other);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void SetUseGlossAlpha(bool bUseGlossAlpha);
	void SetUseSpecularMap(bool bUseSpecularMap);
	void SetSpecularWeight(float SpecularWeight);
	void SetSpecularGloss(float SpecularGloss);

	void SetSpecularColor(Math::Vector3 SpecularColor);
	void SetSpecularColorByElement(float R, float G, float B);
	void SetUseNormalMap(bool bUseNormalMap);

	void SetNormalMapWeight(float NormalMapWeight);

	Math::Vector4 UseGlossAlphaUseSpecularMapAndSpecularWeightGloss;
	Math::Vector4 SpecularColorAndUseNormalMap;
	float NormalMapWeight;
};

class Color3Buffer : public RenderingResource
{
public:
	Color3Buffer(DirectX::XMFLOAT3 InputColor = { 0.2f, 0.2f, 0.2f });
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void SetColor(DirectX::XMFLOAT3 InputColor);
	void SetColorByComponent(float R, float G, float B);

	DirectX::XMFLOAT3 Color;
};

class TransformBuffer : public RenderingResource
{
public:
	TransformBuffer(DirectX::XMMATRIX _ModelViewProj = {});
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void InitializeParentReference(const Entity& _Entity) noexcept override;

	void SetModelViewProj(DirectX::XMMATRIX _ModelViewProj);
	void SetModelViewProjByComponent(DirectX::XMMATRIX View, DirectX::XMMATRIX Proj);

	DirectX::XMMATRIX ModelViewProj;

private:
	void updateBind(custom::CommandContext& BaseContext);
	void updateModelViewProj(custom::CommandContext& BaseContext);
private:
	const Entity* pParentEntity = nullptr;
};