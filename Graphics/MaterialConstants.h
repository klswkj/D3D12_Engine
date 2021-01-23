#pragma once
#include "RenderingResource.h"

class IEntity;
class IWindow;

#define DEFAULT_DIFFUSE_COLOR  Math::Vector3(0.18f, 0.18f, 0.18f)
#define DEFAULT_SPECULAR_COLOR Math::Vector3(0.45f, 0.45f, 0.45f)
#define DEFAULT_SPECULAR_GLOSS 128.0f
#define DEFAULT_WEIGHT         1.0f

// Keep Sync in HLSL.

/*
D3D12Engine::EndFrame()
ModelComponentWindow::RenderWindow()
Node::Accept(Window);
IEntity::Accept(Window);
Technique::Accept(Window);
Step::Accept(Window);
RenderingResource::Accept(Window);
TechniqueProbe::VisitBufer();
TP::OnVisitBuffer();

					 <-> TechniqueProbe
ModelComponentWIndow <-> MP
IWindow       <-> TP
*/

class IControllerConstant : public RenderingResource
{
public:
	virtual ~IControllerConstant() {}

	virtual void SetDiffuseColor(Math::Vector3 DiffuseColor) {}
	virtual void SetDiffuseColorByElement(float R, float G, float B) {}

	virtual void SetSpecularColor(Math::Vector3 SpecularColor) {}
	virtual void SetSpecularColorByElement(float R, float G, float B) {}
	virtual void SetSpecularWeight(float SpecularWeight) {}

	virtual void SetUseSpecularMap(bool bUseSpecularMap) {}
	virtual void SetSpecularGloss(float SpecularGloss) {}
	virtual void SetUseGlossAlpha(bool bUseGlossAlpha) {}

	virtual void SetUseNormalMap(bool bUseNormalMap) {}
	virtual void SetNormalMapWeight(float NormalMapWeight) {}

protected:
	bool m_bDiffuse  = false;;
	bool m_bSpecular = false;
	bool m_Normal    = false;
};

// Has none Texture
class ControllerConstants0 : public IControllerConstant
{
public:
	ControllerConstants0
	(
		Math::Vector3 DiffuseColor  = DEFAULT_DIFFUSE_COLOR,
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR,
		float SpecularGloss         = DEFAULT_SPECULAR_GLOSS,
		float SpecularWeight        = 1.0f
	);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	void SetDiffuseColor(Math::Vector3 DiffuseColor) override;
	void SetDiffuseColorByElement(float R, float G, float B) override;
	void SetSpecularGloss(float SpecularGloss) override;

	void SetSpecularColor(Math::Vector3 SpecularColor) override;
	void SetSpecularColorByElement(float R, float G, float B) override;
	void SetSpecularWeight(float SpecularWeight) override;

	struct Data
	{
		Math::Vector4 DiffuseColorAndSpecularGloss;
		Math::Vector4 SpecularColorAndWeight;
	};
	Data m_Data;
};

// Diffuse
class ControllerConstants1 : public IControllerConstant
{
public:
	ControllerConstants1
	(
		Math::Vector3 SpecularColor = DEFAULT_SPECULAR_COLOR, 
		float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		float SpecularWeight = 1.0f
	);

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	void SetSpecularColor(Math::Vector3 SpecularColor) override;
	void SetSpecularColorByElement(float R, float G, float B) override;
	void SetSpecularWeight(float SpecularWeight) override;

	void SetSpecularGloss(float SpecularGloss) override;
	struct Data
	{
		Math::Vector4 SpecularColorAndWeight;
		float SpecularGloss;
	};
	Data m_Data;
};

// Specular
class ControllerConstants2 : public IControllerConstant
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

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	// Data Handle Method
	void SetDiffuseColor(Math::Vector3 DiffuseColor) override;
	void SetDiffuseColorByElement(float R, float G, float B) override;
	void SetUseGlossAlpha(bool bUseGlossAlpha) override;

	void SetUseSpecularMap(bool bUseSpecularMap) override;
	void SetSpecularWeight(float SpecularWeight) override;
	void SetSpecularGloss(float SpecularGloss) override;

	void SetSpecularColor(Math::Vector3 SpecularColor) override;
	void SetSpecularColorByElement(float R, float G, float B) override;

	struct Data
	{
		Math::Vector4 DiffuseColorAndUseGlossAlpha;
		Math::Vector3 UseSpecularMapSpecularWeightGloss;
		DirectX::XMFLOAT3 SpecularColor;
	};

	Data m_Data;
};

// Diffuse Specular
class ControllerConstants3 : public IControllerConstant
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

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	void SetUseGlossAlpha(bool bUseGlossAlpha) override;
	void SetUseSpecularMap(bool bUseSpecularMap) override;
	void SetSpecularWeight(float SpecularWeight) override;
	void SetSpecularGloss(float SpecularGloss) override;

	void SetSpecularColor(Math::Vector3 SpecularColor) override;
	void SetSpecularColorByElement(float R, float G, float B) override;

	struct Data
	{
		Math::Vector4 UseGlossAlphaUseSpecularMapAndSpecularWeightGloss;
		Math::Vector3 SpecularColor;
	};
	Data m_Data;
};

// Normal
class ControllerConstants4 : public IControllerConstant
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

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	// Data Handle Method
	void SetDiffuseColor(Math::Vector3 DiffuseColor) override;
	void SetDiffuseColorByElement(float R, float G, float B) override;
	void SetSpecularWeight(float SpecularWeight) override;

	void SetSpecularColor(Math::Vector3 SpecularColor) override;
	void SetSpecularColorByElement(float R, float G, float B) override;
	void SetSpecularGloss(float SpecularGloss) override;

	void SetUseNormalMap(bool bUseNormalMap) override;
	void SetNormalMapWeight(float NormalMapWeight) override;
	struct Data
	{
		Math::Vector4 DiffuseColorAndSpecularWeight;
		Math::Vector4 SpecularColorGloss;
		Math::Vector2 UseNormalMapWeight;
	};
	Data m_Data;
};

// Diffuse Normal
class ControllerConstants5 : public IControllerConstant
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

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	void SetSpecularWeight(float SpecularWeight) override;
	void SetSpecularGloss(float SpecularGloss) override;
	void SetUseNormalMap(bool bUseNormalMap) override;
	void SetNormalMapWeight(float NormalMapWeight) override;

	void SetSpecularColor(Math::Vector3 SpecularColor) override;
	void SetSpecularColorByElement(float R, float G, float B) override;

	struct Data
	{
		Math::Vector4 SpecularWeightGlossUseNormalMapNormalWeight;
		Math::Vector3 SpecularColor;
	};
	Data m_Data;
};

// Specular Normal
class ControllerConstants6 : public IControllerConstant
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

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	void SetDiffuseColor(Math::Vector3 DiffuseColor) override;
	void SetDiffuseColorByElement(float R, float G, float B) override;
	void SetUseGlossAlpha(bool bUseGlossAlpha) override;

	void SetUseSpecularMap(bool bUseSpecularMap) override;
	void SetSpecularWeight(float SpecularWeight) override;
	void SetSpecularGloss(float SpecularGloss) override;
	void SetUseNormalMap(bool bUseNormalMap) override;

	void SetSpecularColor(Math::Vector3 SpecularColor) override;
	void SetSpecularColorByElement(float R, float G, float B) override;
	void SetNormalMapWeight(float NormalMapWeight) override;

	struct Data
	{
		Math::Vector4 DiffuseColorUseGlossAlpha;
		Math::Vector4 UseSpecularMapWeightGlossAndNormalMap;
		Math::Vector4 SpecularColorAndNormalMapWeight;
	};
	Data m_Data;
};

class ControllerConstants7 : public IControllerConstant
{
public:
	// UseGlossAlpha : 1065353216 => 1.0f 
	// UseSpecularMap : ""
	// SpecularWeight : 1
	// SpecularGloss : 3
	// SplecularColor : 0.0f, 0.0f, 0.0f
	// UseNormalMap : 106535216
	// NormalMapWeight : 1
	// Padding : -
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

	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;
	void RenderingWindow(IWindow& _IWindow) override;

	void SetUseGlossAlpha(bool bUseGlossAlpha) override;
	void SetUseSpecularMap(bool bUseSpecularMap) override;
	void SetSpecularWeight(float SpecularWeight) override;
	void SetSpecularGloss(float SpecularGloss) override;

	void SetSpecularColor(Math::Vector3 SpecularColor) override;
	void SetSpecularColorByElement(float R, float G, float B) override;
	void SetUseNormalMap(bool bUseNormalMap) override;

	void SetNormalMapWeight(float NormalMapWeight) override;

	struct Data
	{
		Math::Vector4 UseGlossAlphaUseSpecularMapAndSpecularWeightGloss;
		Math::Vector4 SpecularColorAndUseNormalMap;
		Math::Vector4 NormalMapWeight;
	};
	Data m_Data;
};

/*
class Color3Buffer : public RenderingResource
{
public:
	Color3Buffer(DirectX::XMFLOAT3 InputColor = { 0.2f, 0.2f, 0.2f });
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void SetColor(DirectX::XMFLOAT3 InputColor);
	void SetColorByComponent(float R, float G, float B);

	DirectX::XMFLOAT3 Color;
};
*/

/*
class TransformBuffer : public RenderingResource
{
public:
	TransformBuffer(DirectX::XMMATRIX _ModelViewProj = {});
	void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT;

	void InitializeParentReference(const IEntity& _Entity) noexcept override;

	void SetModelViewProj(DirectX::XMMATRIX _ModelViewProj);
	void SetModelViewProjByComponent(DirectX::XMMATRIX View, DirectX::XMMATRIX Proj);

	DirectX::XMMATRIX ModelViewProj;

private:
	void updateBind(custom::CommandContext& BaseContext);
	void updateModelViewProj(custom::CommandContext& BaseContext);
private:
	const IEntity* pParentEntity = nullptr;
};
*/