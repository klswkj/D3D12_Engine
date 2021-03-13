#include "stdafx.h"
#include "ObjModelManager.h"
#include "OBJFileManager.h"
#include "MasterRenderGraph.h"
#include "UAVBuffer.h"
#include "ObjectFilterFlag.h"
#include "CustomImgui.h"
#include "TextureManager.h"


ObjModelManager::ObjModelManager(float ModelSize)
{
	std::vector<std::string>& FileNames = OBJFileManager::g_FileNames;
	size_t FileSize{ FileNames.size() };

	ASSERT(FileSize, "Cannot find any ObjFiles.");

	m_Models.reserve(FileSize);

	for (const auto& FileName : FileNames)
	{
		m_Models.emplace_back(Model{ FileName, ModelSize });
	}

	ComputeBoundingBox();

	m_ModelComponentWindows.reserve(FileSize);

	for (size_t i = 0; i < FileSize; ++i)
	{
		m_ModelComponentWindows.emplace_back(FileNames[i]);
	}

	m_ActiveModelIndex = 0;
}

ObjModelManager::~ObjModelManager()
{
	custom::StructuredBuffer::DestroyAllVertexBuffer();
	custom::ByteAddressBuffer::DestroyIndexAllBuffer();
}

void ObjModelManager::LinkTechniques(MasterRenderGraph& _MasterRenderGraph)
{
	TextureManager::AllTextureResourceTransition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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

void ObjModelManager::RenderComponentWindows()
{
	const size_t ModelSize           = m_Models.size();
	const size_t ComponentWindowSize = m_ModelComponentWindows.size();

	ASSERT(ModelSize == ComponentWindowSize);

	if (ModelSize == 0)
	{
		return;
	}

	ImGui::Begin("Model");
	ImGui::Columns(2, 0, true);

	// ÄÞº¸¹Ú½º
	ImGui::Text("Select Model");
	if (ImGui::BeginCombo("", m_Models[m_ActiveModelIndex].GetName().c_str()))
	{
		for (size_t ModelIndex = 0; ModelIndex < m_Models.size(); ++ModelIndex)
		{
			const bool bSelected = (ModelIndex == m_ActiveModelIndex);

			if (ImGui::Selectable(m_Models[ModelIndex].GetName().c_str(), bSelected))
			{
				m_ActiveModelIndex = ModelIndex;
			}
		}
		ImGui::EndCombo();
	}

	// "CustomImgui.h"
	// Draw Custom line.
	// ImGui::CenteredSeparator(100);
	// ImGui::SameLine();
	// ImGui::CenteredSeparator();
	
	m_ModelComponentWindows[m_ActiveModelIndex].ShowWindow(m_Models[m_ActiveModelIndex]);

	ImGui::End();
}

void ObjModelManager::ComputeBoundingBox()
{
	for (size_t IModel = 0; IModel < m_Models.size(); ++IModel)
	{
		BoundingBox& temp = (BoundingBox&)m_Models[IModel].GetBoundingBox();

		m_BoundingBox.MinPoint = Math::Min(temp.MinPoint, m_BoundingBox.MinPoint);
		m_BoundingBox.MaxPoint = Math::Max(temp.MaxPoint, m_BoundingBox.MaxPoint);
	}
}
ObjModelManager::BoundingBox& ObjModelManager::GetBoundingBox()
{
	return m_BoundingBox;
}