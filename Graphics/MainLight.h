#pragma once
#include "FilledPrimitiveSphere.h"
// #include "ShadowCamera.h"
#include "Vector.h"

class MainLight
{
	friend class custom::CommandContext;
public:
	MainLight
	(
		float LightOrientation = -0.5f,
		float LightInclination = 0.75f,
		Math::Vector3 LightColor = { 1.0f, 1.0f, 1.0f },
		Math::Vector3 AmbientColor = { 1.0f, 1.0f, 1.0f },
		Math::Vector3 ShadowCenter = { -500.0f, -1000.0f, -1000.0f },
		Math::Vector3 ShadowBounds = { 5000.0f, 3000.0f, 3000.0f }
	);

	const Math::Vector3 GetMainLightDirection();
	// const Math::Matrix4& GetShadowMatrix();

private:
	// FilledPrimitiveSphere m_Mesh; // Entity

	//float costheta = cosf(m_SunOrientation);
	//float sintheta = sinf(m_SunOrientation);
	//float cosphi = cosf(m_SunInclination * 3.14159f * 0.5f);
	//float sinphi = sinf(m_SunInclination * 3.14159f * 0.5f);
	//m_SunDirection = Normalize(Vector3(costheta * cosphi, sinphi, sintheta * cosphi));
	float m_LightOrientation;
	float m_LightInclination;
	Math::Vector3 m_LightColor;
	Math::Vector3 m_AmbientColor;
	float m_LightIntensity;
	float m_AmbientIntensity;

	Math::Vector3 m_ShadowCenter;
	Math::Vector3 m_ShadowBounds;
};

