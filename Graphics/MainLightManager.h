#pragma once
#include "MainLight"


class MainLightManager
{
public:

	void RenderWindow();
	void AddMainLight();

private:
	std::vector<MainLight> m_MainLights;
};