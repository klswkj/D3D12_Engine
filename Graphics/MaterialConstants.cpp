#include "stdafx.h"
#include "MaterialConstants.h"
#include "CommandContext.h"
#include "Entity.h"
#include "Camera.h"

#ifndef CONTROLLER_ROOT_INDEX
#define CONTROLLER_ROOT_INDEX 3u
#endif
#ifndef FLAT_SHADER_TRANSFORM_ROOT_INDEX
#define FLAT_SHADER_TRANSFORM_ROOT_INDEX 0u
#endif

#ifndef FLAT_SHADER_COLOR_ROOT_INDEX
#define FLAT_SHADER_COLOR_ROOT_INDEX 1u
#endif

namespace
{
	constexpr size_t SIZE_FLOAT = sizeof(float);
	constexpr size_t SIZE_VECTOR4 = sizeof(const Math::Vector4);
	constexpr size_t SIZE_VECTOR3 = SIZE_VECTOR4;
	constexpr size_t SIZE_FLOAT2 = sizeof(DirectX::XMFLOAT2);
	constexpr size_t SIZE_FLOAT3 = sizeof(DirectX::XMFLOAT3);
}

#pragma region CONTROLLERCONSTANTS

ControllerConstants0::ControllerConstants0(const Math::Vector3& DiffuseColor, const Math::Vector3& SpecularColor, const float SpecularGloss, const float SpecularWeight)
{
	SetDiffuseColor(DiffuseColor);
	SetSpecularGloss(SpecularGloss);
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
}
ControllerConstants1::ControllerConstants1(const Math::Vector3& SpecularColor, const float SpecularGloss, const float SpecularWeight)
{
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);

	m_bDiffuse = true;
}
ControllerConstants2::ControllerConstants2(const Math::Vector3& DiffuseColor, const Math::Vector3& SpecularColor, const float SpecularWeight, const float SpecularGloss, const bool bUseSpecularMap, const bool bUseGlossAlpha)
{
	SetDiffuseColor(DiffuseColor);
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetUseSpecularMap(bUseSpecularMap);
	SetUseGlossAlpha(bUseGlossAlpha);

	m_bSpecular = true;
}
ControllerConstants3::ControllerConstants3(const Math::Vector3& SpecularColor, const float SpecularGloss, const float SpecularWeight, const bool bUseSpecularMap, const bool bUseGlossAlpha)
{
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetUseSpecularMap(bUseSpecularMap);
	SetUseGlossAlpha(bUseGlossAlpha);

	m_bDiffuse  = true;
	m_bSpecular = true;
}
ControllerConstants4::ControllerConstants4(const Math::Vector3& DiffuseColor, const Math::Vector3& SpecularColor, const float SpecularGloss, const float SpecularWeight, const float NormalMapWeight, const bool bUseNormalMap)
{
	SetDiffuseColor(DiffuseColor);
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetNormalMapWeight(NormalMapWeight);
	SetUseNormalMap(bUseNormalMap);

	m_Normal = true;
}
ControllerConstants5::ControllerConstants5(const Math::Vector3& SpecularColor, const float SpecularGloss, const float SpecularWeight, const float NormalMapWeight, const bool bUseNormalMap)
{
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetNormalMapWeight(NormalMapWeight);
	SetUseNormalMap(bUseNormalMap);

	m_bDiffuse = true;
	m_Normal = true;
}
ControllerConstants6::ControllerConstants6(const Math::Vector3& DiffuseColor, const Math::Vector3& SpecularColor, const float SpecularGloss, const float SpecularWeight, const float NormalMapWeight, const bool bUseGlossAlpha, const bool bUseSpecularMap, const bool bUseNormalMap)
{
	SetDiffuseColor(DiffuseColor);
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetNormalMapWeight(NormalMapWeight);
	SetUseGlossAlpha(bUseGlossAlpha);
	SetUseSpecularMap(bUseSpecularMap);
	SetUseNormalMap(bUseNormalMap);

	m_bSpecular = true;
	m_Normal = true;
}
ControllerConstants7::ControllerConstants7(const Math::Vector3& SpecularColor, const float SpecularGloss, const float SpecularWeight, const float NormalMapWeight, const bool bUseGlossAlpha, const bool bUseSpecularMap, const bool bUseNormalMap)
{
	SetSpecularColor(SpecularColor);
	SetSpecularWeight(SpecularWeight);
	SetSpecularGloss(SpecularGloss);
	SetNormalMapWeight(NormalMapWeight);
	SetUseGlossAlpha(bUseGlossAlpha);
	SetUseSpecularMap(bUseSpecularMap);
	SetUseNormalMap(bUseNormalMap);

	m_bDiffuse  = true;
	m_bSpecular = true;
	m_Normal    = true;
}

void ControllerConstants0::SetDiffuseColor(const Math::Vector3& DiffuseColor)
{
	m_Data.DiffuseColorAndSpecularGloss = DiffuseColor;
}
void ControllerConstants0::SetDiffuseColorByElement(const float R, const float G, const float B)
{
	m_Data.DiffuseColorAndSpecularGloss.SetX(R);
	m_Data.DiffuseColorAndSpecularGloss.SetY(G);
	m_Data.DiffuseColorAndSpecularGloss.SetZ(B);
}
void ControllerConstants0::SetSpecularGloss(const float SpecularGloss)
{
	m_Data.DiffuseColorAndSpecularGloss.SetW(SpecularGloss);
}
void ControllerConstants0::SetSpecularColor(const Math::Vector3& SpecularColor)
{
	m_Data.SpecularColorAndWeight = SpecularColor;
}
void ControllerConstants0::SetSpecularColorByElement(const float R, const float G, const float B)
{
	m_Data.SpecularColorAndWeight.SetX(R);
	m_Data.SpecularColorAndWeight.SetY(G);
	m_Data.SpecularColorAndWeight.SetZ(B);
}
void ControllerConstants0::SetSpecularWeight(const float SpecularWeight)
{
	m_Data.SpecularColorAndWeight.SetW(SpecularWeight);
}

void ControllerConstants1::SetSpecularColor(const Math::Vector3& _SpecularColor)
{
	m_Data.SpecularColorAndWeight = _SpecularColor;
}
void ControllerConstants1::SetSpecularColorByElement(const float R, const float G, const float B)
{
	m_Data.SpecularColorAndWeight.SetX(R);
	m_Data.SpecularColorAndWeight.SetY(G);
	m_Data.SpecularColorAndWeight.SetZ(B);
}
void ControllerConstants1::SetSpecularWeight(const float SpecularWeight)
{
	m_Data.SpecularColorAndWeight.SetW(SpecularWeight);
}
void ControllerConstants1::SetSpecularGloss(const float _SpecularGloss)
{
	m_Data.SpecularGloss = _SpecularGloss;
}

void ControllerConstants2::SetDiffuseColor(const Math::Vector3& _DiffuseColor)
{
	m_Data.DiffuseColorAndUseGlossAlpha = _DiffuseColor;
}
void ControllerConstants2::SetDiffuseColorByElement(const float R, const float G, const float B)
{
	m_Data.DiffuseColorAndUseGlossAlpha.SetX(R);
	m_Data.DiffuseColorAndUseGlossAlpha.SetX(G);
	m_Data.DiffuseColorAndUseGlossAlpha.SetX(B);
}
void ControllerConstants2::SetUseGlossAlpha(const bool bUse)
{
	m_Data.DiffuseColorAndUseGlossAlpha.SetZ(bUse);
}
void ControllerConstants2::SetUseSpecularMap(const bool bUse)
{
	m_Data.UseSpecularMapSpecularWeightGloss.SetX(bUse);
}
void ControllerConstants2::SetSpecularWeight(const float Weight)
{
	m_Data.UseSpecularMapSpecularWeightGloss.SetY(Weight);
}
void ControllerConstants2::SetSpecularGloss(const float SpecularGloss)
{
	m_Data.UseSpecularMapSpecularWeightGloss.SetZ(SpecularGloss);
}
void ControllerConstants2::SetSpecularColor(const Math::Vector3& _SpecularColor)
{
	m_Data.SpecularColor = _SpecularColor;
}
void ControllerConstants2::SetSpecularColorByElement(const float R, const float G, const float B)
{
	m_Data.SpecularColor.x = R;
	m_Data.SpecularColor.y = G;
	m_Data.SpecularColor.z = B;
}

void ControllerConstants3::SetUseGlossAlpha(const bool bUseGlossAlpha)
{
	m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetX(bUseGlossAlpha);
}
void ControllerConstants3::SetUseSpecularMap(const bool bUseSpecularMap)
{
	m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetY(bUseSpecularMap);
}
void ControllerConstants3::SetSpecularWeight(const float SpecularWeight)
{
	m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetZ(SpecularWeight);
}
void ControllerConstants3::SetSpecularGloss(const float SpecularGloss)
{
	m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetW(SpecularGloss);
}
void ControllerConstants3::SetSpecularColor(const Math::Vector3& _SpecularColor)
{
	m_Data.SpecularColor = _SpecularColor;
}
void ControllerConstants3::SetSpecularColorByElement(const float R, const float G, const float B)
{
	m_Data.SpecularColor.SetX(R);
	m_Data.SpecularColor.SetY(G);
	m_Data.SpecularColor.SetZ(B);
}

void ControllerConstants4::SetDiffuseColor(const Math::Vector3& DiffuseColor)
{
	m_Data.DiffuseColorAndSpecularWeight = DiffuseColor;
}
void ControllerConstants4::SetDiffuseColorByElement(const float R, const float G, const float B)
{
	m_Data.DiffuseColorAndSpecularWeight.SetX(R);
	m_Data.DiffuseColorAndSpecularWeight.SetY(G);
	m_Data.DiffuseColorAndSpecularWeight.SetZ(B);
}
void ControllerConstants4::SetSpecularWeight(const float SpecularWeight)
{
	m_Data.DiffuseColorAndSpecularWeight.SetW(SpecularWeight);
}
void ControllerConstants4::SetSpecularColor(const Math::Vector3& SpecularColor)
{
	m_Data.SpecularColorGloss = SpecularColor;
}
void ControllerConstants4::SetSpecularColorByElement(const float R, const float G, const float B)
{
	m_Data.SpecularColorGloss.SetX(R);
	m_Data.SpecularColorGloss.SetY(G);
	m_Data.SpecularColorGloss.SetZ(B);
}
void ControllerConstants4::SetSpecularGloss(const float SpecularGloss)
{
	m_Data.SpecularColorGloss.SetW(SpecularGloss);
}
void ControllerConstants4::SetUseNormalMap(const bool bUseNormalMap)
{
	m_Data.UseNormalMapWeight.SetX(bUseNormalMap);
}
void ControllerConstants4::SetNormalMapWeight(const float NormalMapWeight)
{
	m_Data.UseNormalMapWeight.SetY(NormalMapWeight);
}

void ControllerConstants5::SetSpecularWeight(const float SpecularWeight)
{
	m_Data.SpecularWeightGlossUseNormalMapNormalWeight.SetX(SpecularWeight);
}
void ControllerConstants5::SetSpecularGloss(const float SpecularGloss)
{
	m_Data.SpecularWeightGlossUseNormalMapNormalWeight.SetY(SpecularGloss);
}
void ControllerConstants5::SetUseNormalMap(const bool bUseNormalMap)
{
	m_Data.SpecularWeightGlossUseNormalMapNormalWeight.SetZ(bUseNormalMap);
}
void ControllerConstants5::SetNormalMapWeight(const float NormalMapWeight)
{
	m_Data.SpecularWeightGlossUseNormalMapNormalWeight.SetW(NormalMapWeight);
}
void ControllerConstants5::SetSpecularColor(const Math::Vector3& _SpecularColor)
{
	m_Data.SpecularColor = _SpecularColor;
}
void ControllerConstants5::SetSpecularColorByElement(const float R, const float G, const float B)
{
	m_Data.SpecularColor.SetX(R);
	m_Data.SpecularColor.SetY(G);
	m_Data.SpecularColor.SetZ(B);
}

void ControllerConstants6::SetDiffuseColor(const Math::Vector3& DiffuseColor)
{
	m_Data.DiffuseColorUseGlossAlpha = DiffuseColor;
}
void ControllerConstants6::SetDiffuseColorByElement(const float R, const float G, const float B)
{
	m_Data.DiffuseColorUseGlossAlpha.SetX(R);
	m_Data.DiffuseColorUseGlossAlpha.SetY(G);
	m_Data.DiffuseColorUseGlossAlpha.SetZ(B);
}
void ControllerConstants6::SetUseGlossAlpha(const bool bUseGlossAlpha)
{
	m_Data.DiffuseColorUseGlossAlpha.SetW(bUseGlossAlpha);
}
void ControllerConstants6::SetUseSpecularMap(const bool bUseSpecularMap)
{
	m_Data.UseSpecularMapWeightGlossAndNormalMap.SetX(bUseSpecularMap);
}
void ControllerConstants6::SetSpecularWeight(const float SpecularWeight)
{
	m_Data.UseSpecularMapWeightGlossAndNormalMap.SetY(SpecularWeight);
}
void ControllerConstants6::SetSpecularGloss(const float SpecularGloss)
{
	m_Data.UseSpecularMapWeightGlossAndNormalMap.SetZ(SpecularGloss);
}
void ControllerConstants6::SetUseNormalMap(const bool bUseNormalMap)
{
	m_Data.UseSpecularMapWeightGlossAndNormalMap.SetZ(bUseNormalMap);
}
void ControllerConstants6::SetSpecularColor(const Math::Vector3& SpecularColor)
{
	m_Data.SpecularColorAndNormalMapWeight = SpecularColor;
}
void ControllerConstants6::SetSpecularColorByElement(const float R, const float G, const float B)
{
	m_Data.SpecularColorAndNormalMapWeight.SetX(R);
	m_Data.SpecularColorAndNormalMapWeight.SetY(G);
	m_Data.SpecularColorAndNormalMapWeight.SetZ(B);
}
void ControllerConstants6::SetNormalMapWeight(const float NormalMapWeight)
{
	m_Data.SpecularColorAndNormalMapWeight.SetW(NormalMapWeight);
}

void ControllerConstants7::SetUseGlossAlpha(const bool bUseGlossAlpha)
{
	m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetX(bUseGlossAlpha);
}
void ControllerConstants7::SetUseSpecularMap(const bool bUseSpecularMap)
{
	m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetY(bUseSpecularMap);
}
void ControllerConstants7::SetSpecularWeight(const float SpecularWeight)
{
	m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetZ(SpecularWeight);
}
void ControllerConstants7::SetSpecularGloss(const float SpecularGloss)
{
	m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetW(SpecularGloss);
}
void ControllerConstants7::SetSpecularColor(const Math::Vector3& SpecularColor)
{
	m_Data.SpecularColorAndUseNormalMap = SpecularColor;
}
void ControllerConstants7::SetSpecularColorByElement(const float R, const float G, const float B)
{
	m_Data.SpecularColorAndUseNormalMap.SetX(R);
	m_Data.SpecularColorAndUseNormalMap.SetY(G);
	m_Data.SpecularColorAndUseNormalMap.SetZ(B);
}
void ControllerConstants7::SetUseNormalMap(const bool bUseNormalMap)
{
	m_Data.SpecularColorAndUseNormalMap.SetW(bUseNormalMap);
}
void ControllerConstants7::SetNormalMapWeight(const float _NormalMapWeight)
{
	// m_Data.NormalMapWeight = _NormalMapWeight;
	m_Data.NormalMapWeight.SetX(_NormalMapWeight);
}
#pragma region BINDCONSTANTS

void ControllerConstants0::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, SIZE_VECTOR4 * 2, &m_Data);
	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(m_Data), &m_Data, commandIndex);
}
void ControllerConstants1::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, SIZE_VECTOR4 + sizeof(const float), &m_Data);
	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(m_Data), &m_Data, commandIndex);
}
void ControllerConstants2::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, SIZE_VECTOR4 * 2 + SIZE_FLOAT3, &m_Data);
	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(m_Data), &m_Data, commandIndex);
}
void ControllerConstants3::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, SIZE_VECTOR4 + SIZE_FLOAT3, &m_Data);
	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(m_Data), &m_Data, commandIndex);
}
void ControllerConstants4::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, SIZE_VECTOR4 * 2 + SIZE_FLOAT2, &m_Data);
	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(m_Data), &m_Data, commandIndex);
}
void ControllerConstants5::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, SIZE_VECTOR4 + SIZE_FLOAT3, &m_Data);
	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(m_Data), &m_Data, commandIndex);
}
void ControllerConstants6::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, SIZE_VECTOR4 * 3, &m_Data); // = sizeof(m_Data)
	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(m_Data), &m_Data, commandIndex);
}
void ControllerConstants7::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

	// graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, SIZE_VECTOR4 * 2 + SIZE_FLOAT, &m_Data);
	graphicsContext.SetDynamicConstantBufferView(CONTROLLER_ROOT_INDEX, sizeof(m_Data), &m_Data, commandIndex);
}

#pragma endregion BINDCONSTANTS


void ControllerConstants0::RenderingWindow(const IWindow& _IWindow)
{
	bool bDirty = false;

	float DiffuseColor[3]   = { m_Data.DiffuseColorAndSpecularGloss.GetX(), m_Data.DiffuseColorAndSpecularGloss.GetY(), m_Data.DiffuseColorAndSpecularGloss.GetZ() };
	float SpecularGloss     = m_Data.DiffuseColorAndSpecularGloss.GetW();

	float SpecularColor[3]  = { m_Data.SpecularColorAndWeight.GetX(), m_Data.SpecularColorAndWeight.GetY(), m_Data.SpecularColorAndWeight.GetZ() };
	float SpecularMapWeight = m_Data.SpecularColorAndWeight.GetW();

	if (bDirty |= ImGui::ColorPicker3("Diffuse Color", DiffuseColor))
	{
		m_Data.DiffuseColorAndSpecularGloss.SetX(DiffuseColor[0]);
		m_Data.DiffuseColorAndSpecularGloss.SetY(DiffuseColor[1]);
		m_Data.DiffuseColorAndSpecularGloss.SetZ(DiffuseColor[2]);
	}
	if (bDirty |= ImGui::ColorPicker3("Specular Color", SpecularColor))
	{
		m_Data.SpecularColorAndWeight.SetX(SpecularColor[0]);
		m_Data.SpecularColorAndWeight.SetY(SpecularColor[1]);
		m_Data.SpecularColorAndWeight.SetZ(SpecularColor[2]);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Weight", &SpecularMapWeight, 0.0f, 2.0f))
	{
		m_Data.SpecularColorAndWeight.SetW(SpecularMapWeight);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Gloss", &SpecularGloss, 1.0f, 100.0f, "%.1f", 1.5f))
	{
		m_Data.DiffuseColorAndSpecularGloss.SetW(SpecularGloss);
	}
	// return bDrity;
}
void ControllerConstants1::RenderingWindow(const IWindow& _IWindow)
{
	bool bDirty = false;

	float SpecularColor[3]  = { m_Data.SpecularColorAndWeight.GetX(), m_Data.SpecularColorAndWeight.GetY(), m_Data.SpecularColorAndWeight.GetZ() };
	float SpecularMapWeight = m_Data.SpecularColorAndWeight.GetW();
	float SpecularGloss     = m_Data.SpecularGloss;

	if (bDirty |= ImGui::ColorPicker3("Specular Color", SpecularColor))
	{
		m_Data.SpecularColorAndWeight.SetX(SpecularColor[0]);
		m_Data.SpecularColorAndWeight.SetY(SpecularColor[1]);
		m_Data.SpecularColorAndWeight.SetZ(SpecularColor[2]);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Weight", &SpecularMapWeight, 0.0f, 2.0f))
	{
		m_Data.SpecularColorAndWeight.SetW(SpecularMapWeight);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Gloss", &SpecularGloss, 1.0f, 100.0f, "%.1f", 1.5f))
	{
		m_Data.SpecularGloss = SpecularGloss;
	}
}
void ControllerConstants2::RenderingWindow(const IWindow& _IWindow)
{
	bool bDirty = false;

	float DiffuseColor[3]   = { m_Data.DiffuseColorAndUseGlossAlpha.GetX(), m_Data.DiffuseColorAndUseGlossAlpha.GetY(), m_Data.DiffuseColorAndUseGlossAlpha.GetZ() };
	bool  bUseSpecularGloss = m_Data.DiffuseColorAndUseGlossAlpha.GetW();
	bool  bUseSpecularMap   = m_Data.UseSpecularMapSpecularWeightGloss.GetX();
	float SpecularMapWeight = m_Data.UseSpecularMapSpecularWeightGloss.GetY();
	float SpecularGloss     = m_Data.UseSpecularMapSpecularWeightGloss.GetZ();
	float SpecularColor[3]  = { m_Data.SpecularColor.x, m_Data.SpecularColor.y, m_Data.SpecularColor.z };

	if (bDirty |= ImGui::ColorPicker3("Diffuse Color", DiffuseColor))
	{
		m_Data.DiffuseColorAndUseGlossAlpha.SetX(DiffuseColor[0]);
		m_Data.DiffuseColorAndUseGlossAlpha.SetY(DiffuseColor[1]);
		m_Data.DiffuseColorAndUseGlossAlpha.SetZ(DiffuseColor[2]);
	}
	if (bDirty |= ImGui::ColorPicker3("Specular Color", SpecularColor))
	{
		m_Data.SpecularColor.x = SpecularColor[0];
		m_Data.SpecularColor.y = SpecularColor[1];
		m_Data.SpecularColor.z = SpecularColor[2];
	}
	if (bDirty |= ImGui::Checkbox("Use Specular Map", &bUseSpecularMap))
	{
		m_Data.UseSpecularMapSpecularWeightGloss.SetX(bUseSpecularMap);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Weight", &SpecularMapWeight, 0.0f, 2.0f))
	{
		m_Data.UseSpecularMapSpecularWeightGloss.SetY(SpecularMapWeight);
	}
	if (bDirty |= ImGui::Checkbox("Use Specular Gloss", &bUseSpecularGloss))
	{
		m_Data.DiffuseColorAndUseGlossAlpha.SetW(bUseSpecularGloss);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Gloss", &SpecularGloss, 1.0f, 100.0f, "%.1f", 1.5f))
	{
		m_Data.UseSpecularMapSpecularWeightGloss.SetZ(SpecularGloss);
	}
}
void ControllerConstants3::RenderingWindow(const IWindow& _IWindow)
{
	bool bDirty = false;

	float SpecularColor[3]  = { m_Data.SpecularColor.GetX(), m_Data.SpecularColor.GetY(), m_Data.SpecularColor.GetZ() };
	bool bUseGlossAlpha     = m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.GetX();
	bool bUseSpecularMap    = m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.GetY();
	float SpecularMapWeight = m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.GetZ();
	float SpecularGloss     = m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.GetW();

	if (bDirty |= ImGui::ColorPicker3("Specular Color", SpecularColor))
	{
		m_Data.SpecularColor.SetX(SpecularColor[0]);
		m_Data.SpecularColor.SetY(SpecularColor[1]);
		m_Data.SpecularColor.SetZ(SpecularColor[2]);
	}

	if (bDirty |= ImGui::Checkbox("Use Specular Map", &bUseSpecularMap))
	{
		m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetY(bUseSpecularMap);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Weight", &SpecularMapWeight, 0.0f, 2.0f))
	{
		m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetZ(SpecularMapWeight);
	}
	if (bDirty |= ImGui::Checkbox("Use Gloss Alpha", &bUseGlossAlpha))
	{
		m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetX(bUseSpecularMap);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Gloss", &SpecularGloss, 1.0f, 100.0f, "%.1f", 1.5f))
	{
		m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetW(SpecularGloss);
	}
}
void ControllerConstants4::RenderingWindow(const IWindow& _IWindow)
{
	bool bDirty = false;

	float DiffuseColor[3]   = { m_Data.DiffuseColorAndSpecularWeight.GetX(), m_Data.DiffuseColorAndSpecularWeight.GetY(), m_Data.DiffuseColorAndSpecularWeight.GetZ() };
	float SpecularMapWeight = m_Data.DiffuseColorAndSpecularWeight.GetW();
	float SpecularColor[3]  = { m_Data.SpecularColorGloss.GetX(), m_Data.SpecularColorGloss.GetY(), m_Data.SpecularColorGloss.GetZ() };
	float SpecularGloss     = m_Data.SpecularColorGloss.GetW();
	bool  bUseNormalMap     = m_Data.UseNormalMapWeight.GetX();
	float NormalMap         = m_Data.UseNormalMapWeight.GetY();

	if (bDirty |= ImGui::ColorPicker3("Diffuse Color", DiffuseColor))
	{
		m_Data.DiffuseColorAndSpecularWeight.SetX(DiffuseColor[0]);
		m_Data.DiffuseColorAndSpecularWeight.SetY(DiffuseColor[1]);
		m_Data.DiffuseColorAndSpecularWeight.SetZ(DiffuseColor[2]);
	}
	if (bDirty |= ImGui::ColorPicker3("Specular Color", SpecularColor))
	{
		m_Data.SpecularColorGloss.SetX(SpecularColor[0]);
		m_Data.SpecularColorGloss.SetY(SpecularColor[1]);
		m_Data.SpecularColorGloss.SetZ(SpecularColor[2]);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Weight", &SpecularMapWeight, 0.0f, 2.0f))
	{
		m_Data.DiffuseColorAndSpecularWeight.SetW(SpecularMapWeight);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Gloss", &SpecularGloss, 1.0f, 100.0f, "%.1f", 1.5f))
	{
		m_Data.SpecularColorGloss.SetW(SpecularGloss);
	}
	if (bDirty |= ImGui::Checkbox("Use Normal Map", &bUseNormalMap))
	{
		m_Data.UseNormalMapWeight.SetX(bUseNormalMap);
	}
	if (bDirty |= ImGui::SliderFloat("Normal Map", &NormalMap, 0.0f, 2.0f))
	{
		m_Data.UseNormalMapWeight.SetY(NormalMap);
	}

}
void ControllerConstants5::RenderingWindow(const IWindow& _IWindow)
{
	bool bDirty = false;

	float SpecularColor[3]  = { m_Data.SpecularColor.GetX(), m_Data.SpecularColor.GetY(), m_Data.SpecularColor.GetZ() };
	float SpecularMapWeight = m_Data.SpecularWeightGlossUseNormalMapNormalWeight.GetX();
	float SpecularGloss     = m_Data.SpecularWeightGlossUseNormalMapNormalWeight.GetY();
	bool  bUseNormalMap     = m_Data.SpecularWeightGlossUseNormalMapNormalWeight.GetZ();
	float NormalMap         = m_Data.SpecularWeightGlossUseNormalMapNormalWeight.GetW();

	if (bDirty |= ImGui::ColorPicker3("Specular Color", SpecularColor))
	{
		m_Data.SpecularColor.SetX(SpecularColor[0]);
		m_Data.SpecularColor.SetY(SpecularColor[1]);
		m_Data.SpecularColor.SetZ(SpecularColor[2]);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Weight", &SpecularMapWeight, 0.0f, 2.0f))
	{
		m_Data.SpecularWeightGlossUseNormalMapNormalWeight.SetX(SpecularMapWeight);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Gloss", &SpecularGloss, 1.0f, 100.0f, "%.1f", 1.5f))
	{
		m_Data.SpecularWeightGlossUseNormalMapNormalWeight.SetY(SpecularGloss);
	}
	if (bDirty |= ImGui::Checkbox("Use Normal Map", &bUseNormalMap))
	{
		m_Data.SpecularWeightGlossUseNormalMapNormalWeight.SetZ(bUseNormalMap);
	}
	if (bDirty |= ImGui::SliderFloat("Normal Map", &NormalMap, 0.0f, 2.0f))
	{
		m_Data.SpecularWeightGlossUseNormalMapNormalWeight.SetW(NormalMap);
	}
}
void ControllerConstants6::RenderingWindow(const IWindow& _IWindow)
{
	bool bDirty = false;

	float DiffuseColor[3]   = { m_Data.DiffuseColorUseGlossAlpha.GetX(), m_Data.DiffuseColorUseGlossAlpha.GetY(), m_Data.DiffuseColorUseGlossAlpha.GetZ() };
	float SpecularColor[3]  = { m_Data.SpecularColorAndNormalMapWeight.GetX(), m_Data.SpecularColorAndNormalMapWeight.GetY(), m_Data.SpecularColorAndNormalMapWeight.GetZ() };
	bool  bUseGlossAlpha    = m_Data.DiffuseColorUseGlossAlpha.GetW();
	bool  bUseSpecularMap   = m_Data.UseSpecularMapWeightGlossAndNormalMap.GetX();
	float SpecularMapWeight = m_Data.UseSpecularMapWeightGlossAndNormalMap.GetY();
	float SpecularGloss     = m_Data.UseSpecularMapWeightGlossAndNormalMap.GetZ();
	bool  bUseNormalMap     = m_Data.UseSpecularMapWeightGlossAndNormalMap.GetW();
	float NormalMapWeight   = m_Data.SpecularColorAndNormalMapWeight.GetW();

	if (bDirty |= ImGui::ColorPicker3("Diffuse Color", DiffuseColor))
	{
		m_Data.DiffuseColorUseGlossAlpha.SetX(DiffuseColor[0]);
		m_Data.DiffuseColorUseGlossAlpha.SetY(DiffuseColor[1]);
		m_Data.DiffuseColorUseGlossAlpha.SetZ(DiffuseColor[2]);
	}
	if (bDirty |= ImGui::ColorPicker3("Specular Color", SpecularColor))
	{
		m_Data.SpecularColorAndNormalMapWeight.SetX(SpecularColor[0]);
		m_Data.SpecularColorAndNormalMapWeight.SetY(SpecularColor[1]);
		m_Data.SpecularColorAndNormalMapWeight.SetZ(SpecularColor[2]);
	}
	if (bDirty |= ImGui::Checkbox("Use Specular Map", &bUseSpecularMap))
	{
		m_Data.UseSpecularMapWeightGlossAndNormalMap.SetX(bUseSpecularMap);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Weight", &SpecularMapWeight, 0.0f, 2.0f))
	{
		m_Data.UseSpecularMapWeightGlossAndNormalMap.SetY(SpecularMapWeight);
	}
	if (bDirty |= ImGui::Checkbox("Use Specular Gloss", &bUseGlossAlpha))
	{
		m_Data.DiffuseColorUseGlossAlpha.SetW(bUseGlossAlpha);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Gloss", &SpecularGloss, 1.0f, 100.0f, "%.1f", 1.5f))
	{
		m_Data.UseSpecularMapWeightGlossAndNormalMap.SetZ(SpecularGloss);
	}
	if (bDirty |= ImGui::Checkbox("Use Normal Map", &bUseNormalMap))
	{
		m_Data.UseSpecularMapWeightGlossAndNormalMap.SetW(bUseNormalMap);
	}
	if (bDirty |= ImGui::SliderFloat("Normal Map", &NormalMapWeight, 0.0f, 2.0f))
	{
		m_Data.SpecularColorAndNormalMapWeight.SetW(NormalMapWeight);
	}
}
void ControllerConstants7::RenderingWindow(const IWindow& _IWindow)
{
	bool bDirty = false;

	float SpecularColor[3]      = { m_Data.SpecularColorAndUseNormalMap.GetX(), m_Data.SpecularColorAndUseNormalMap.GetY(), m_Data.SpecularColorAndUseNormalMap.GetZ() };
	bool  bUseGlossAlpha        = m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.GetX();
	bool  bUseSpecularMap       = m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.GetY();
	float SpecularMapWeight     = m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.GetZ();
	float SpecularGloss         = m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.GetW();
	bool  bUseNormalMap         = m_Data.SpecularColorAndUseNormalMap.GetW();
	float NormalMapWeight       = m_Data.NormalMapWeight.GetX();

	if (bDirty |= ImGui::ColorPicker3("Specular Color", SpecularColor))
	{
		m_Data.SpecularColorAndUseNormalMap.SetX(SpecularColor[0]);
		m_Data.SpecularColorAndUseNormalMap.SetY(SpecularColor[1]);
		m_Data.SpecularColorAndUseNormalMap.SetZ(SpecularColor[2]);
	}
	if (bDirty |= ImGui::Checkbox("Use Specular Map", &bUseSpecularMap))
	{
		m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetY(bUseSpecularMap);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Weight", &SpecularMapWeight, 0.0f, 2.0f))
	{
		m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetZ(SpecularMapWeight);
	}
	if (bDirty |= ImGui::Checkbox("Use Specular Gloss", &bUseGlossAlpha))
	{
		m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetX(bUseGlossAlpha);
	}
	if (bDirty |= ImGui::SliderFloat("Specular Gloss", &SpecularGloss, 1.0f, 100.0f, "%.1f", 1.5f))
	{
		m_Data.UseGlossAlphaUseSpecularMapAndSpecularWeightGloss.SetW(SpecularGloss);
	}
	if (bDirty |= ImGui::Checkbox("Use Normal Map", &bUseNormalMap))
	{
		m_Data.SpecularColorAndUseNormalMap.SetW(bUseNormalMap);
	}
	if (bDirty |= ImGui::SliderFloat("Normal Map", &NormalMapWeight, 0.0f, 2.0f))
	{
		m_Data.NormalMapWeight.SetX(NormalMapWeight);
	}
}

#pragma endregion CONTROLLERCONSTANTS

/*
Color3Buffer::Color3Buffer(DirectX::XMFLOAT3 InputColor)
{
	Color = InputColor;
}
void Color3Buffer::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();
	graphicsContext.SetDynamicConstantBufferView(FLAT_SHADER_COLOR_ROOT_INDEX, sizeof(DirectX::XMFLOAT3), (void*)&Color);
}

void Color3Buffer::SetColor(DirectX::XMFLOAT3 InputColor)
{
	Color = InputColor;
}
void Color3Buffer::SetColorByComponent(const float R, const float G, const float B)
{
	Color.x = R;
	Color.y = G;
	Color.z = B;
}
*/
/*
TransformBuffer::TransformBuffer(DirectX::XMMATRIX _ModelViewProj)
{
	ModelViewProj = _ModelViewProj;
}
void TransformBuffer::Bind(custom::CommandContext& BaseContext, const uint8_t commandIndex) DEBUG_EXCEPT
{
	updateBind(BaseContext);
}

void TransformBuffer::InitializeParentReference(const IEntity& _Entity) noexcept
{
	pParentEntity = &_Entity;

}

void TransformBuffer::SetModelViewProj(DirectX::XMMATRIX _ViewProj)
{
	ASSERT(pParentEntity != nullptr);
	const DirectX::XMMATRIX Model = pParentEntity->GetTransform();

	ModelViewProj = Model * _ViewProj;
}
void TransformBuffer::SetModelViewProjByComponent(DirectX::XMMATRIX View, DirectX::XMMATRIX Proj)
{
	ASSERT(pParentEntity != nullptr);
	const DirectX::XMMATRIX Model = pParentEntity->GetTransform();

	ModelViewProj = Model * View * Proj;
}

void TransformBuffer::updateBind(custom::CommandContext& BaseContext, const uint8_t commandIndex)
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

	const DirectX::XMMATRIX Model = pParentEntity->GetTransform();
	ModelViewProj = Model * pMainCamera->GetViewProjMatrix();
}
*/