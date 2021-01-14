#include "stdafx.h"
#include "MainLight.h"
#include "ShadowCamera.h"
#include "Matrix4.h"
#include "BufferManager.h"

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
	bDirty = true;
}

const Math::Vector3 MainLight::GetMainLightDirection()
{
	if (bDirty)
	{
		float costheta = cosf(m_LightOrientation);
		float sintheta = sinf(m_LightOrientation);
		float cosphi = cosf(m_LightInclination * 3.14159f * 0.5f);
		float sinphi = sinf(m_LightInclination * 3.14159f * 0.5f);

		m_LightDirectionVector = Math::Normalize(Math::Vector3(costheta * cosphi, sinphi, sintheta * cosphi));
		bDirty = false;
	}

	return m_LightDirectionVector;
}

const Math::Vector3 MainLight::GetMainLightColor()
{
	return m_LightColor * m_LightIntensity;
}

const Math::Vector3 MainLight::GetAmbientColor()
{
	return m_AmbientColor * m_AmbientIntensity;
}

const Math::Vector3 MainLight::GetShadowCenter()
{
	return m_ShadowCenter;
}
const Math::Vector3 MainLight::GetShadowBounds()
{
	return m_ShadowBounds;
}

const Math::Matrix4& MainLight::GetShadowMatrix()
{
	static ShadowCamera TempCamera;

	TempCamera.UpdateShadowMatrix(GetMainLightDirection(), GetShadowCenter(), GetShadowBounds(),
		bufferManager::g_ShadowBuffer.GetWidth(), bufferManager::g_ShadowBuffer.GetHeight(), 16);

	return TempCamera.GetShadowMatrix();
}

void MainLight::Up()
{
	if (m_LightOrientation <= 190.0f)
	{
		m_LightOrientation += 1.0f;
		bDirty = true;
	}
}
void MainLight::Down()
{
	if (-190.0f <= m_LightOrientation)
	{
		m_LightOrientation -= 1.0f;
		bDirty = true;
	}
}

void MainLight::Rotate1()
{
	if (0.1f <= m_LightInclination)
	{
		m_LightInclination -= 0.05f;
		bDirty = true;
	}
}
void MainLight::Rotate2()
{
	if (m_LightInclination <= 0.9f)
	{
		m_LightInclination += 0.05f;
		bDirty = true;
	}
}