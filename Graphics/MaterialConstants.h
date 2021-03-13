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

	virtual void SetDiffuseColor(const Math::Vector3& DiffuseColor) {};
	virtual void SetDiffuseColorByElement(const float R, const float G, const float B) {};

	virtual void SetSpecularColor(const Math::Vector3& SpecularColor) {};
	virtual void SetSpecularColorByElement(const float R, const float G, const float B) {};
	virtual void SetSpecularWeight(const float SpecularWeight) {};

	virtual void SetUseSpecularMap(const bool bUseSpecularMap) {};
	virtual void SetSpecularGloss(const float SpecularGloss) {};
	virtual void SetUseGlossAlpha(const bool bUseGlossAlpha) {};

	virtual void SetUseNormalMap(const bool bUseNormalMap) {};
	virtual void SetNormalMapWeight(const float NormalMapWeight) {};

protected:
	bool m_bDiffuse  = false;;
	bool m_bSpecular = false;
	bool m_Normal    = false;
};

// Has none Texture
class ControllerConstants0 final : public IControllerConstant
{
public:
	ControllerConstants0
	(
		const Math::Vector3& DiffuseColor  = DEFAULT_DIFFUSE_COLOR,
		const Math::Vector3& SpecularColor = DEFAULT_SPECULAR_COLOR,
		const float SpecularGloss         = DEFAULT_SPECULAR_GLOSS,
		const float SpecularWeight        = 1.0f
	);

	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	void SetDiffuseColor(const Math::Vector3& DiffuseColor) final;
	void SetDiffuseColorByElement(const float R, const float G, const float B) final;
	void SetSpecularGloss(const float SpecularGloss) final;

	void SetSpecularColor(const Math::Vector3& SpecularColor) final;
	void SetSpecularColorByElement(const float R, const float G, const float B) final;
	void SetSpecularWeight(const float SpecularWeight) final;

	struct Data
	{
		Math::Vector4 DiffuseColorAndSpecularGloss;
		Math::Vector4 SpecularColorAndWeight;
	};
	Data m_Data;
};

// Diffuse
class ControllerConstants1 final : public IControllerConstant
{
public:
	ControllerConstants1
	(
		const Math::Vector3& SpecularColor = DEFAULT_SPECULAR_COLOR, 
		const float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		const float SpecularWeight = 1.0f
	);

	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	void SetSpecularColor(const Math::Vector3& SpecularColor) final;
	void SetSpecularColorByElement(const float R, const float G, const float B) final;
	void SetSpecularWeight(const float SpecularWeight) final;

	void SetSpecularGloss(const float SpecularGloss) final;
	struct Data
	{
		Math::Vector4 SpecularColorAndWeight;
		float SpecularGloss;
	};
	Data m_Data;
};

// Specular
class ControllerConstants2 final : public IControllerConstant
{
public:
	ControllerConstants2
	(
		const Math::Vector3& DiffuseColor = DEFAULT_DIFFUSE_COLOR,
		const Math::Vector3& SpecularColor = DEFAULT_SPECULAR_COLOR, 
		const float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		const float SpecularWeight = 1.0f, 
		const bool bUseSpecularMap = true, 
		const bool bUseGlossAlpha = true
	);

	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	// Data Handle Method
	void SetDiffuseColor(const Math::Vector3& DiffuseColor) final;
	void SetDiffuseColorByElement(const float R, const float G, const float B) final;
	void SetUseGlossAlpha(const bool bUseGlossAlpha) final;

	void SetUseSpecularMap(const bool bUseSpecularMap) final;
	void SetSpecularWeight(const float SpecularWeight) final;
	void SetSpecularGloss(const float SpecularGloss) final;

	void SetSpecularColor(const Math::Vector3& SpecularColor) final;
	void SetSpecularColorByElement(const float R, const float G, const float B) final;

	struct Data
	{
		Math::Vector4 DiffuseColorAndUseGlossAlpha;
		Math::Vector3 UseSpecularMapSpecularWeightGloss;
		DirectX::XMFLOAT3 SpecularColor;
	};

	Data m_Data;
};

// Diffuse Specular
class ControllerConstants3 final : public IControllerConstant
{
public:
	ControllerConstants3
	(
		const Math::Vector3& SpecularColor = DEFAULT_SPECULAR_COLOR,
		const float SpecularGloss = DEFAULT_SPECULAR_GLOSS,
		const float SpecularWeight = 1.0f,
		const bool bUseSpecularMap = true,
		const bool bUseGlossAlpha = true
	);

	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	void SetUseGlossAlpha(const bool bUseGlossAlpha) final;
	void SetUseSpecularMap(const bool bUseSpecularMap) final;
	void SetSpecularWeight(const float SpecularWeight) final;
	void SetSpecularGloss(const float SpecularGloss) final;

	void SetSpecularColor(const Math::Vector3& SpecularColor) final;
	void SetSpecularColorByElement(const float R, const float G, const float B) final;

	struct Data
	{
		Math::Vector4 UseGlossAlphaUseSpecularMapAndSpecularWeightGloss;
		Math::Vector3 SpecularColor;
	};
	Data m_Data;
};

// Normal
class ControllerConstants4 final : public IControllerConstant
{
public:
	ControllerConstants4
	(
		const Math::Vector3& DiffuseColor = DEFAULT_DIFFUSE_COLOR,
		const Math::Vector3& SpecularColor = DEFAULT_SPECULAR_COLOR,
		const float SpecularGloss = DEFAULT_SPECULAR_GLOSS,
		const float SpecularWeight = 1.0f,
		const float NormalMapWeight = 1.0f,
		const bool bUseNormalMap = true
	);

	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	// Data Handle Method
	void SetDiffuseColor(const Math::Vector3& DiffuseColor) final;
	void SetDiffuseColorByElement(const float R, const float G, const float B) final;
	void SetSpecularWeight(const float SpecularWeight) final;

	void SetSpecularColor(const Math::Vector3& SpecularColor) final;
	void SetSpecularColorByElement(const float R, const float G, const float B) final;
	void SetSpecularGloss(const float SpecularGloss) final;

	void SetUseNormalMap(const bool bUseNormalMap) final;
	void SetNormalMapWeight(const float NormalMapWeight) final;
	struct Data
	{
		Math::Vector4 DiffuseColorAndSpecularWeight;
		Math::Vector4 SpecularColorGloss;
		Math::Vector2 UseNormalMapWeight;
	};
	Data m_Data;
};

// Diffuse Normal
class ControllerConstants5 final : public IControllerConstant
{
public:
	ControllerConstants5
	(
		const Math::Vector3& SpecularColor = DEFAULT_SPECULAR_COLOR, 
		const float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		const float SpecularWeight = 1.0f, 
		const float NormalMapWeight = 1.0f, 
		const bool bUseNormalMap = true
	);

	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	void SetSpecularWeight(const float SpecularWeight) final;
	void SetSpecularGloss(const float SpecularGloss) final;
	void SetUseNormalMap(const bool bUseNormalMap) final;
	void SetNormalMapWeight(const float NormalMapWeight) final;

	void SetSpecularColor(const Math::Vector3& SpecularColor) final;
	void SetSpecularColorByElement(const float R, const float G, const float B) final;

	struct Data
	{
		Math::Vector4 SpecularWeightGlossUseNormalMapNormalWeight;
		Math::Vector3 SpecularColor;
	};
	Data m_Data;
};

// Specular Normal
class ControllerConstants6 final : public IControllerConstant
{
public:
	ControllerConstants6
	(
		const Math::Vector3& DiffuseColor = DEFAULT_DIFFUSE_COLOR,
		const Math::Vector3& SpecularColor = DEFAULT_SPECULAR_COLOR,
		const float SpecularGloss = DEFAULT_SPECULAR_GLOSS,
		const float SpecularWeight = 1.0f,
		const float NormalMapWeight = 1.0f,
		const bool bUseGlossAlpha = true,
		const bool bUseSpecularMap = true,
		const bool bUseNormalMap = true
	);

	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	void SetDiffuseColor(const Math::Vector3& DiffuseColor) final;
	void SetDiffuseColorByElement(const float R, const float G, const float B) final;
	void SetUseGlossAlpha(const bool bUseGlossAlpha) final;

	void SetUseSpecularMap(const bool bUseSpecularMap) final;
	void SetSpecularWeight(const float SpecularWeight) final;
	void SetSpecularGloss(const float SpecularGloss) final;
	void SetUseNormalMap(const bool bUseNormalMap) final;

	void SetSpecularColor(const Math::Vector3& SpecularColor) final;
	void SetSpecularColorByElement(const float R, const float G, const float B) final;
	void SetNormalMapWeight(const float NormalMapWeight) final;

	struct Data
	{
		Math::Vector4 DiffuseColorUseGlossAlpha;
		Math::Vector4 UseSpecularMapWeightGlossAndNormalMap;
		Math::Vector4 SpecularColorAndNormalMapWeight;
	};
	Data m_Data;
};

class ControllerConstants7 final : public IControllerConstant
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
		const Math::Vector3& SpecularColor = DEFAULT_SPECULAR_COLOR, 
		const float SpecularGloss = DEFAULT_SPECULAR_GLOSS, 
		const float SpecularWeight = 1.0f, 
		const float NormalMapWeight = 1.0f, 
		const bool bUseGlossAlpha = true, 
		const bool bUseSpecularMap = true,
		const bool bUseNormalMap = true
	);

	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;
	void RenderingWindow(const IWindow& _IWindow) final;

	void SetUseGlossAlpha(const bool bUseGlossAlpha) final;
	void SetUseSpecularMap(const bool bUseSpecularMap) final;
	void SetSpecularWeight(const float SpecularWeight) final;
	void SetSpecularGloss(const float SpecularGloss) final;

	void SetSpecularColor(const Math::Vector3& SpecularColor) final;
	void SetSpecularColorByElement(const float R, const float G, const float B) final;
	void SetUseNormalMap(const bool bUseNormalMap) final;

	void SetNormalMapWeight(const float NormalMapWeight) final;

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
	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;

	void SetColor(DirectX::XMFLOAT3 InputColor);
	void SetColorByComponent(const float R, const float G, const float B);

	DirectX::XMFLOAT3 Color;
};
*/

/*
class TransformBuffer : public RenderingResource
{
public:
	TransformBuffer(DirectX::XMMATRIX _ModelViewProj = {});
	void Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT final;

	void InitializeParentReference(IEntity& _Entity) noexcept final;

	void SetModelViewProj(DirectX::XMMATRIX _ModelViewProj);
	void SetModelViewProjByComponent(DirectX::XMMATRIX View, DirectX::XMMATRIX Proj);

	DirectX::XMMATRIX ModelViewProj;

private:
	void updateBind(custom::CommandContext& BaseContext);
	void updateModelViewProj(custom::CommandContext& BaseContext);
private:
	IEntity* pParentEntity = nullptr;
};
*/