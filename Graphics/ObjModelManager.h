#pragma once
#include "Model.h"
#include "ModelComponentWindow.h"

class MasterRenderGraph;
enum class eObjectFilterFlag;

class ObjModelManager
{
public:
	ObjModelManager();
	
	void LinkTechniques(MasterRenderGraph& _MasterRenderGraph); // 원하는 Entity내의 Techiniques안의 Step이 Pass 찾아가게함.
	void Submit(eObjectFilterFlag Filter); // Pass에 맡게 pDrawable을 분배함.
	void ShowComponentWindows();
private:
	std::vector<Model> m_Models;
	std::vector<ModelComponentWindow> m_ComponentWindows;
};