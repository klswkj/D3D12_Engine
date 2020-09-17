#pragma once
#include "Model.h"

class MasterRenderGraph;
enum class eObjectFilterFlag;

class ObjModelManager
{
	ObjModelManager();
	
	void LinkTechniques(MasterRenderGraph& _MasterRenderGraph);
	void Submit(eObjectFilterFlag Filter);
	void ShowComponentWindows();
private:
	std::vector<Model> m_Models;
	std::vector<ComponentWindow> m_ComponentWindows;
};