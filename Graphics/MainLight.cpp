#include "stdafx.h"
#include "MainLight.h"
#include "ShadowCamera.h"
#include "Matrix4.h"

MainLight::MainLight
(
	float LightOrientation,
	float LightInclination,
	Math::Vector3 LightColor,
	Math::Vector3 AmbientColor,
	Math::Vector3 ShadowCenter,
	Math::Vector3 ShadowBounds
)
	: m_LightOrientation(LightOrientation),
	m_LightInclination(LightInclination),
	m_LightColor(LightColor),
	m_AmbientColor(AmbientColor),
	m_LightIntensity(1.0f),
	m_AmbientIntensity(1.0f),
	m_ShadowCenter(ShadowCenter),
	m_ShadowBounds(ShadowBounds)
{

}

const Math::Vector3 MainLight::GetMainLightDirection()
{
	float costheta = cosf(m_LightOrientation);
	float sintheta = sinf(m_LightOrientation);
	float cosphi = cosf(m_LightInclination * 3.14159f * 0.5f);
	float sinphi = sinf(m_LightInclination * 3.14159f * 0.5f);

	Math::Vector3 ret = Math::Normalize(Math::Vector3(costheta * cosphi, sinphi, sintheta * cosphi));

	return ret;
}