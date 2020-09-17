#include "stdafx.h"
#include "PassWindow.h"
#include "imgui.h"
#include "Pass.h"

void PassWindow::RegisterPass(Pass* pass)
{
	m_Passes.push_back(pass);
}
void PassWindow::RenderPassWindow()
{
	ImGui::Begin(m_Name, nullptr, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Columns(2, nullptr, true);
	ImGui::BeginChild("", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);
	listAllPasses();
	ImGui::EndChild();
	ImGui::NextColumn();

	if (m_pSelectedPass != nullptr)
	{
		// hw3d->TestModelProbe.h->Line.116
		// m_pSelectedPass->RenderSubWindow();
	}

	ImGui::End();
}

void PassWindow::listAllPasses()
{
	const uint32_t selectedPassIndex = (m_pSelectedPass == nullptr) ? -1 : m_SelectedPassIndex;

	for (const auto& pass : m_Passes)
	{
		const int node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | 
			ImGuiTreeNodeFlags_Leaf | ((pass->m_PassIndex == selectedPassIndex) ? ImGuiTreeNodeFlags_Selected : 0);
			// (node.HasChildren() ? 0 : ImGuiTreeNodeFlags_Leaf);

		if (ImGui::TreeNodeEx((void*)pass->m_PassIndex, node_flags, pass->GetRegisteredName()))
		{
			if (ImGui::IsItemClicked())
			{
				m_pSelectedPass = pass;
			}
			ImGui::TreePop();
		}
	}
}