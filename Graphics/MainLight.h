#pragma once
#include "FilledPrimitiveSphere.h"

namespace custom
{
	class CommandContextManager;
	class CommandContext;
}

class MainLight
{
	friend custom::CommandContextManager;
	friend custom::CommandContext;
public:
	MainLight
	(
		float LightOrientation = 0.0f,
		float LightInclination = 0.70f,
		Math::Vector3 LightColor = { 1.0f, 1.0f, 1.0f },
		Math::Vector3 AmbientColor = { 1.0f, 1.0f, 1.0f },
		Math::Vector3 ShadowCenter = { -500.0f, -1000.0f, -1000.0f },
		Math::Vector3 ShadowBounds = { 5000.0f, 3000.0f, 3000.0f }
	);

	const Math::Vector3 GetMainLightDirection() const;
	const Math::Vector3 GetMainLightColor() const;
	const float GetMainLightIntensity() const;
	const Math::Vector3 GetAmbientColor() const;
	const float GetAmibientIntensity() const;
	const Math::Vector3 GetShadowCenter() const;
	const Math::Vector3 GetShadowBounds() const;
	const Math::Matrix4& GetShadowMatrix();

	void Up();
	void Down();
	void Rotate1();
	void Rotate2();
private:
	// FilledPrimitiveSphere m_Mesh; // IEntity

	Math::Vector3 m_LightColor;
	Math::Vector3 m_AmbientColor;
	float m_LightOrientation;
	float m_LightInclination;
	float m_LightIntensity;
	float m_AmbientIntensity;

	Math::Vector3 m_ShadowCenter;
	Math::Vector3 m_ShadowBounds;

	mutable Math::Vector3 m_LightDirectionVector;
	mutable bool bDirty;
};

