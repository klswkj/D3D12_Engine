#include "stdafx.h"
#include "MainLightManager.h"
#include "MathBasic.h"

MainLightManager::MainLightManager(float LightOrientation /*= -0.5f*/, float LightInclination /*= 0.75f*/)
{
	m_MainLights.push_back(MainLight(LightOrientation, LightInclination));

	ASSERT(m_MainLights.size());
}

void MainLightManager::RenderWindow()
{
}

void MainLightManager::addMainLight()
{

}

std::vector<MainLight>* MainLightManager::GetContainer()
{
	return &m_MainLights;
}