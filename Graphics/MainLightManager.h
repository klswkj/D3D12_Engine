#pragma once
#include "MainLight.h"
#include "Matrix4.h"

// MainLightManager를, 
// 그리는 Pass들한테 컨테이너를 건네줘서
// MainLightManager::std::vector<MainLight>의 사이즈만큼 돌려야됨

// MainLight는 
//


class MainLightManager
{
public:
	MainLightManager(float LightOrientation = -0.5f, float LightInclination = 0.75f);
	void RenderWindow(); // 여기서 addMainLight()를 불러와야지 -> RenderComponentWindow()가 아니고, RenderWindow()가 맞다.

	std::vector<MainLight>* GetContainer(); // Send to MasterRenderGraph and Passes

private:
	void addMainLight();

private:
	std::vector<MainLight> m_MainLights;
};