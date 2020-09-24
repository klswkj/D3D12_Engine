#include "stdafx.h"
#include "ObjModelManager.h"
#include "OBJFileManager.h"
#include "Model.h"
#include "MasterRenderGraph.h"
#include "ModelComponentWindow.h"
#include "ObjectFilterFlag.h"

ObjModelManager::ObjModelManager()
{
	std::vector<std::string>& FileNames = OBJFileManager::g_FileNames;
	size_t FileSize{ FileNames.size() };

	m_Models.reserve(FileSize);

	for (const auto& FileName : FileNames)
	{
		m_Models.emplace_back(FileName);
	}

	m_ComponentWindows.reserve(FileSize);

	for (size_t i = 0; i < FileSize; ++i)
	{
		m_ComponentWindows.emplace_back(FileNames[i]);
	}
}

void ObjModelManager::LinkTechniques(MasterRenderGraph& _MasterRenderGraph)
{
	for (auto& _Model : m_Models)
	{
		_Model.LinkTechniques(_MasterRenderGraph);
	}
}

void ObjModelManager::Submit(eObjectFilterFlag Filter)
{
	for (auto& _Model : m_Models)
	{
		_Model.Submit(Filter);
	}
}

void ObjModelManager::ShowComponentWindows()
{
	const size_t ModelSize = m_Models.size();
	const size_t ComponentWindowSize = m_ComponentWindows.size();

	ASSERT(ModelSize == ComponentWindowSize);

	for (size_t i = 0; i < ModelSize; ++i)
	{
		m_ComponentWindows[i].ShowWindow(m_Models[i]);
	}
}
