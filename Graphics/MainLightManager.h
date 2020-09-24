#pragma once
#include "MainLight.h"
#include "Matrix4.h"

// MainLightManager��, 
// �׸��� Pass������ �����̳ʸ� �ǳ��༭
// MainLightManager::std::vector<MainLight>�� �����ŭ �����ߵ�

// MainLight�� 
//


class MainLightManager
{
public:
	MainLightManager(float LightOrientation = -0.5f, float LightInclination = 0.75f);
	void RenderWindow(); // ���⼭ addMainLight()�� �ҷ��;��� -> RenderComponentWindow()�� �ƴϰ�, RenderWindow()�� �´�.

	std::vector<MainLight>* GetContainer(); // Send to MasterRenderGraph and Passes

private:
	void addMainLight();

private:
	std::vector<MainLight> m_MainLights;
};