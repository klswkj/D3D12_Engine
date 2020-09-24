#pragma once
#include "Model.h"
#include "ModelComponentWindow.h"

class MasterRenderGraph;
enum class eObjectFilterFlag;

class ObjModelManager
{
public:
	ObjModelManager();
	
	void LinkTechniques(MasterRenderGraph& _MasterRenderGraph); // ���ϴ� Entity���� Techiniques���� Step�� Pass ã�ư�����.
	void Submit(eObjectFilterFlag Filter); // Pass�� �ð� pDrawable�� �й���.
	void ShowComponentWindows();
private:
	std::vector<Model> m_Models;
	std::vector<ModelComponentWindow> m_ComponentWindows;
};