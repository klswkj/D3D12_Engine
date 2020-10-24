#include "stdafx.h"
#include "MaterialConstants.h"
#include "CommandContext.h"
#include "Entity.h"
#include "Camera.h"

#ifndef CONTROLLER_ROOT_INDEX
#define CONTROLLER_ROOT_INDEX 2
#endif
#ifndef FLAT_SHADER_TRANSFORM_ROOT_INDEX
#define FLAT_SHADER_TRANSFORM_ROOT_INDEX 0
#endif

#ifndef FLAT_SHADER_COLOR_ROOT_INDEX
#define FLAT_SHADER_COLOR_ROOT_INDEX 1
#endif

#pragma region CONTROLLERCONSTANTS

ControllerConstants0::ControllerConstants0(Math::Vector3 DiffuseColor, Math::Vector3 SpecularColor, float SpecularGloss, float SpecularWeight)
{
	SetDiffuseColor(DiffuseColor);
	SetSpecularGloss(SpecularGloss);
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
}

ControllerConstants1::ControllerConstants1(Math::Vector3 SpecularColor, float SpecularGloss, float SpecularWeight)
{
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
}
ControllerConstants2::ControllerConstants2(Math::Vector3 DiffuseColor, Math::Vector3 SpecularColor, float SpecularWeight, float SpecularGloss, bool bUseSpecularMap, bool bUseGlossAlpha)
{
	SetDiffuseColor(DiffuseColor);
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetUseSpecularMap(bUseSpecularMap);
	SetUseGlossAlpha(bUseGlossAlpha);
}
ControllerConstants3::ControllerConstants3(Math::Vector3 SpecularColor, float SpecularGloss, float SpecularWeight, bool bUseSpecularMap, bool bUseGlossAlpha)
{
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetUseSpecularMap(bUseSpecularMap);
	SetUseGlossAlpha(bUseGlossAlpha);
}
ControllerConstants4::ControllerConstants4(Math::Vector3 DiffuseColor, Math::Vector3 SpecularColor, float SpecularGloss, float SpecularWeight, float NormalMapWeight, bool bUseNormalMap)
{
	SetDiffuseColor(DiffuseColor);
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetNormalMapWeight(NormalMapWeight);
	SetUseNormalMap(bUseNormalMap);
}
ControllerConstants5::ControllerConstants5(Math::Vector3 SpecularColor, float SpecularGloss, float SpecularWeight, float NormalMapWeight, bool bUseNormalMap)
{
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetNormalMapWeight(NormalMapWeight);
	SetUseNormalMap(bUseNormalMap);
}
ControllerConstants6::ControllerConstants6(Math::Vector3 DiffuseColor, Math::Vector3 SpecularColor, float SpecularGloss, float SpecularWeight, float NormalMapWeight, bool bUseGlossAlpha, bool bUseSpecularMap, bool bUseNormalMap)
{
	SetDiffuseColor(DiffuseColor);
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetNormalMapWeight(NormalMapWeight);
	SetUseGlossAlpha(bUseGlossAlpha);
	SetUseSpecularMap(bUseSpecularMap);
	SetUseNormalMap(bUseNormalMap);
}
ControllerConstants7::ControllerConstants7(Math::Vector3 SpecularColor, float SpecularGloss, float SpecularWeight, float NormalMapWeight, bool bUseGlossAlpha, bool bUseSpecularMap, bool bUseNormalMap)
{
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetNormalMapWeight(NormalMapWeight);
	SetUseGlossAlpha(bUseGlossAlpha);
	SetUseSpecularMap(bUseSpecularMap);
	SetUseNormalMap(bUseNormalMap);
}
/*
ControllerConstants0::ControllerConstants0(ControllerConstants0&& Other)
{

}

ControllerConstants1::ControllerConstants1(ControllerConstants1&& Other)
{

}
ControllerConstants2::ControllerConstants2(ControllerConstants2&& Other)
{

}
ControllerConstants3::ControllerConstants3(ControllerConstants3&& Other)
{

}
ControllerConstants4::ControllerConstants4(ControllerConstants4&& Other)
{

}
ControllerConstants5::ControllerConstants5(ControllerConstants5&& Other)
{

}
ControllerConstants6::ControllerConstants6(ControllerConstants6&& Other)
{

}
ControllerConstants7::ControllerConstants7(ControllerConstants7&& Other)
{

}
*/
void ControllerConstants0::SetDiffuseColor(Math::Vector3 DiffuseColor)
{
	DiffuseColorAndSpecularGloss = DiffuseColor;
}
void ControllerConstants0::SetDiffuseColorByElement(float R, float G, float B)
{
	DiffuseColorAndSpecularGloss.SetX(R);
	DiffuseColorAndSpecularGloss.SetY(G);
	DiffuseColorAndSpecularGloss.SetZ(B);
}
void ControllerConstants0::SetSpecularGloss(float SpecularGloss)
{
	DiffuseColorAndSpecularGloss.SetW(SpecularGloss);
}

void ControllerConstants0::SetSpecularColor(Math::Vector3 SpecularColor)
{
	SpecularColorAndWeight = SpecularColor;
}
void ControllerConstants0::SetSpecularColorByElement(float R, float G, float B)
{
	SpecularColorAndWeight.SetX(R);
	SpecularColorAndWeight.SetY(G);
	SpecularColorAndWeight.SetZ(B);
}
void ControllerConstants0::SetSpecularWeight(float SpecularWeight)
{
	SpecularColorAndWeight.SetW(SpecularWeight);
}

void ControllerConstants1::SetSpecularColor(Math::Vector3 _SpecularColor)
{
	SpecularColorAndWeight = _SpecularColor;
}

void ControllerConstants1::SetSpecularColorByElement(float R, float G, float B)
{
	SpecularColorAndWeight.SetX(R);
	SpecularColorAndWeight.SetY(G);
	SpecularColorAndWeight.SetZ(B);
}

void ControllerConstants1::SetSpecularWeight(float SpecularWeight)
{
	SpecularColorAndWeight.SetW(SpecularWeight);
}

void ControllerConstants1::SetSpecularGloss(float _SpecularGloss)
{
	SpecularGloss = _SpecularGloss;
}


void ControllerConstants2::SetDiffuseColor(Math::Vector3 _DiffuseColor)
{
	DiffuseColorAndUseGlossAlpha = _DiffuseColor;
}
void ControllerConstants2::SetDiffuseColorByElement(float R, float G, float B)
{
	DiffuseColorAndUseGlossAlpha.SetX(R);
	DiffuseColorAndUseGlossAlpha.SetX(G);
	DiffuseColorAndUseGlossAlpha.SetX(B);
}
void ControllerConstants2::SetUseGlossAlpha(bool bUse)
{
	DiffuseColorAndUseGlossAlpha.SetZ(bUse);
}

void ControllerConstants2::SetUseSpecularMap(bool bUse)
{
	UseSpecularMapSpecularWeightGloss.SetX(bUse);
}
void ControllerConstants2::SetSpecularWeight(float Weight)
{
	UseSpecularMapSpecularWeightGloss.SetY(Weight);
}
void ControllerConstants2::SetSpecularGloss(float SpecularGloss)
{
	UseSpecularMapSpecularWeightGloss.SetZ(SpecularGloss);
}

void ControllerConstants2::SetSpecularColor(Math::Vector3 _SpecularColor)
{
	SpecularColor = _SpecularColor;
}
void ControllerConstants2::SetSpecularColorByElement(float R, float G, float B)
{
	SpecularColor.x = R;
	SpecularColor.y = G;
	SpecularColor.z = B;
}


void ControllerConstants3::SetUseGlossAlpha(bool bUseGlossAlpha)
{
	UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetX(bUseGlossAlpha);
}
void ControllerConstants3::SetUseSpecularMap(bool bUseSpecularMap)
{
	UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetY(bUseSpecularMap);
}
void ControllerConstants3::SetSpecularWeight(float SpecularWeight)
{
	UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetZ(SpecularWeight);
}
void ControllerConstants3::SetSpecularGloss(float SpecularGloss)
{
	UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetW(SpecularGloss);
}

void ControllerConstants3::SetSpecularColor(Math::Vector3 _SpecularColor)
{
	SpecularColor = _SpecularColor;
}
void ControllerConstants3::SetSpecularColorByElement(float R, float G, float B)
{
	SpecularColor.x = R;
	SpecularColor.y = G;
	SpecularColor.z = B;
}



void ControllerConstants4::SetDiffuseColor(Math::Vector3 DiffuseColor)
{
	DiffuseColorAndSpecularWeight = DiffuseColor;
}
void ControllerConstants4::SetDiffuseColorByElement(float R, float G, float B)
{
	DiffuseColorAndSpecularWeight.SetX(R);
	DiffuseColorAndSpecularWeight.SetY(G);
	DiffuseColorAndSpecularWeight.SetZ(B);
}
void ControllerConstants4::SetSpecularWeight(float SpecularWeight)
{
	DiffuseColorAndSpecularWeight.SetW(SpecularWeight);
}

void ControllerConstants4::SetSpecularColor(Math::Vector3 SpecularColor)
{
	SpecularColorGloss = SpecularColor;
}
void ControllerConstants4::SetSpecularColorByElement(float R, float G, float B)
{
	SpecularColorGloss.SetX(R);
	SpecularColorGloss.SetY(G);
	SpecularColorGloss.SetZ(B);
}
void ControllerConstants4::SetSpecularGloss(float SpecularGloss)
{
	SpecularColorGloss.SetW(SpecularGloss);
}

void ControllerConstants4::SetUseNormalMap(bool bUseNormalMap)
{
	UseNormalMapWeight.x = bUseNormalMap;
}
void ControllerConstants4::SetNormalMapWeight(float NormalMapWeight)
{
	UseNormalMapWeight.y = NormalMapWeight;
}



void ControllerConstants5::SetSpecularWeight(float SpecularWeight)
{
	SpecularWeightGlossUseNormalMapNormalWeight.SetX(SpecularWeight);
}
void ControllerConstants5::SetSpecularGloss(float SpecularGloss)
{
	SpecularWeightGlossUseNormalMapNormalWeight.SetY(SpecularGloss);
}
void ControllerConstants5::SetUseNormalMap(bool bUseNormalMap)
{
	SpecularWeightGlossUseNormalMapNormalWeight.SetZ(bUseNormalMap);
}
void ControllerConstants5::SetNormalMapWeight(float NormalMapWeight)
{
	SpecularWeightGlossUseNormalMapNormalWeight.SetW(NormalMapWeight);
}

void ControllerConstants5::SetSpecularColor(Math::Vector3 _SpecularColor)
{
	SpecularColor = _SpecularColor;
}
void ControllerConstants5::SetSpecularColorByElement(float R, float G, float B)
{
	SpecularColor.x = R;
	SpecularColor.y = G;
	SpecularColor.z = B;
}



void ControllerConstants6::SetDiffuseColor(Math::Vector3 DiffuseColor)
{
	DiffuseColorUseGlossAlpha = DiffuseColor;
}
void ControllerConstants6::SetDiffuseColorByElement(float R, float G, float B)
{
	DiffuseColorUseGlossAlpha.SetX(R);
	DiffuseColorUseGlossAlpha.SetY(G);
	DiffuseColorUseGlossAlpha.SetZ(B);
}
void ControllerConstants6::SetUseGlossAlpha(bool bUseGlossAlpha)
{
	DiffuseColorUseGlossAlpha.SetW(bUseGlossAlpha);
}

void ControllerConstants6::SetUseSpecularMap(bool bUseSpecularMap)
{
	UseSpecularMapWeightGlossAndNormalMap.SetX(bUseSpecularMap);
}
void ControllerConstants6::SetSpecularWeight(float SpecularWeight)
{
	UseSpecularMapWeightGlossAndNormalMap.SetY(SpecularWeight);
}
void ControllerConstants6::SetSpecularGloss(float SpecularGloss)
{
	UseSpecularMapWeightGlossAndNormalMap.SetZ(SpecularGloss);
}
void ControllerConstants6::SetUseNormalMap(bool bUseNormalMap)
{
	UseSpecularMapWeightGlossAndNormalMap.SetZ(bUseNormalMap);
}

void ControllerConstants6::SetSpecularColor(Math::Vector3 SpecularColor)
{
	SpecularColorAndNormalMapWeight = SpecularColor;
}
void ControllerConstants6::SetSpecularColorByElement(float R, float G, float B)
{
	SpecularColorAndNormalMapWeight.SetX(R);
	SpecularColorAndNormalMapWeight.SetY(G);
	SpecularColorAndNormalMapWeight.SetZ(B);
}
void ControllerConstants6::SetNormalMapWeight(float NormalMapWeight)
{
	SpecularColorAndNormalMapWeight.SetW(NormalMapWeight);
}


void ControllerConstants7::SetUseGlossAlpha(bool bUseGlossAlpha)
{
	UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetX(bUseGlossAlpha);
}
void ControllerConstants7::SetUseSpecularMap(bool bUseSpecularMap)
{
	UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetY(bUseSpecularMap);
}
void ControllerConstants7::SetSpecularWeight(float SpecularWeight)
{
	UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetZ(SpecularWeight);
}
void ControllerConstants7::SetSpecularGloss(float SpecularGloss)
{
	UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetW(SpecularGloss);
}

void ControllerConstants7::SetSpecularColor(Math::Vector3 SpecularColor)
{
	SpecularColorAndUseNormalMap = SpecularColor;
}
void ControllerConstants7::SetSpecularColorByElement(float R, float G, float B)
{
	SpecularColorAndUseNormalMap.SetX(R);
	SpecularColorAndUseNormalMap.SetY(G);
	SpecularColorAndUseNormalMap.SetZ(B);
}
void ControllerConstants7::SetUseNormalMap(bool bUseNormalMap)
{
	SpecularColorAndUseNormalMap.SetW(bUseNormalMap);
}

void ControllerConstants7::SetNormalMapWeight(float _NormalMapWeight)
{
	NormalMapWeight = _NormalMapWeight;
}

#pragma region BINDCONSTANTS

void ControllerConstants0::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(this), this);
}
void ControllerConstants1::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(this), this);
}

void ControllerConstants2::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(this), this);
}

void ControllerConstants3::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(this), this);
}

void ControllerConstants4::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(this), this);
}

void ControllerConstants5::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(this), this);
}

void ControllerConstants6::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(this), this);
}

void ControllerConstants7::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(this), this);
}
#pragma endregion BINDCONSTANTS

#pragma endregion CONTROLLERCONSTANTS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Color3Buffer::Color3Buffer(DirectX::XMFLOAT3 InputColor)
{
	Color = InputColor;
}
void Color3Buffer::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetDynamicConstantBufferView(FLAT_SHADER_COLOR_ROOT_INDEX, sizeof(DirectX::XMFLOAT3), (void*)&Color);
}

void Color3Buffer::SetColor(DirectX::XMFLOAT3 InputColor)
{
	Color = InputColor;
}
void Color3Buffer::SetColorByComponent(float R, float G, float B)
{
	Color.x = R;
	Color.y = G;
	Color.z = B;
}


TransformBuffer::TransformBuffer(DirectX::XMMATRIX _ModelViewProj)
{
	ModelViewProj = _ModelViewProj;
}
void TransformBuffer::Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	updateBind(BaseContext);
}

void TransformBuffer::InitializeParentReference(const Entity& _Entity) noexcept
{
	pParentEntity = &_Entity;

}

void TransformBuffer::SetModelViewProj(DirectX::XMMATRIX _ViewProj)
{
	ASSERT(pParentEntity != nullptr);
	const DirectX::XMMATRIX Model = pParentEntity->GetTransformXM();

	ModelViewProj = Model * _ViewProj;
}
void TransformBuffer::SetModelViewProjByComponent(DirectX::XMMATRIX View, DirectX::XMMATRIX Proj)
{
	ASSERT(pParentEntity != nullptr);
	const DirectX::XMMATRIX Model = pParentEntity->GetTransformXM();

	ModelViewProj = Model * View * Proj;
}

void TransformBuffer::updateBind(custom::CommandContext& BaseContext)
{
	updateModelViewProj(BaseContext);

	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetDynamicConstantBufferView(FLAT_SHADER_TRANSFORM_ROOT_INDEX, sizeof(DirectX::XMMATRIX), &ModelViewProj);

}
void TransformBuffer::updateModelViewProj(custom::CommandContext& BaseContext)
{
	ASSERT(pParentEntity != nullptr);
	// ASSERT(BaseContext.GetpMainCamera() != nullptr);

	// TODO : Make DirtyFlag

	const Camera* pMainCamera = BaseContext.GetpMainCamera();

	const DirectX::XMMATRIX Model = pParentEntity->GetTransformXM();
	ModelViewProj = Model * pMainCamera->GetViewProjMatrix();
}