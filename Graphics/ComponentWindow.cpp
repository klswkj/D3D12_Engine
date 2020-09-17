#include "stdafx.h"
#include "ComponentWindow.h"
#include "Model.h"
// #include "ModelPart.h"

void ComponentWindow::ShowWindow(Model& model)
{
	ImGui::Begin(name.c_str(), nullptr, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Columns(2, nullptr, true);
	ImGui::BeginChild("Test1", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);
	model.Accept(*this);
	ImGui::EndChild();
	ImGui::NextColumn();

	if (pSelectedNode != nullptr)
	{
		bool bDirty = false;
		const auto dcheck = [&bDirty](bool changed) {bDirty = bDirty || changed; };
		auto& tf = resolveTransform();
		ImGui::TextColored({ 0.4f,1.0f,0.6f,1.0f }, "Translation");
		dcheck(ImGui::SliderFloat("X", &tf.x, -60.f, 60.f));
		dcheck(ImGui::SliderFloat("Y", &tf.y, -60.f, 60.f));
		dcheck(ImGui::SliderFloat("Z", &tf.z, -60.f, 60.f));
		ImGui::TextColored({ 0.4f,1.0f,0.6f,1.0f }, "Orientation");
		dcheck(ImGui::SliderAngle("X-rotation", &tf.xRot, -180.0f, 180.0f));
		dcheck(ImGui::SliderAngle("Y-rotation", &tf.yRot, -180.0f, 180.0f));
		dcheck(ImGui::SliderAngle("Z-rotation", &tf.zRot, -180.0f, 180.0f));
		if (bDirty)
		{
			pSelectedNode->SetAppliedTransform
			(
				DirectX::XMMatrixRotationX(tf.xRot) *
				DirectX::XMMatrixRotationY(tf.yRot) *
				DirectX::XMMatrixRotationZ(tf.zRot) *
				DirectX::XMMatrixTranslation(tf.x, tf.y, tf.z)
			);
		}

		// AddonWindow addon;
		// pSelectedNode->PushAddon(addon);
	}

	ImGui::End();
}

bool ComponentWindow::PushNode(ModelPart& node)
{
	// If there is no selected node, set selectedId to an impossible value
	const uint32_t selectedId = (pSelectedNode == nullptr) ? -1 : pSelectedNode->GetId();

	// Build up flags for current node
	const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | // Only open when clicking on the arrow part. If ImGuiTreeNodeFlags_OpenOnDoubleClick is also set, single-click arrow or double-click all box to open.
		ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_FramePadding      |
		((node.GetId() == selectedId) ? ImGuiTreeNodeFlags_Selected : 0) | // Draw as selected
		(node.HasChildren() ? 0 : ImGuiTreeNodeFlags_Leaf);                // No collapsing, no arrow (use as a convenience for leaf nodes).

	// render this node
	const auto expanded = ImGui::TreeNodeEx
	(
		(void*)(uint32_t)node.GetId(),
		node_flags, node.GetName()
	);

	// processing for selecting node
	if (ImGui::IsItemClicked())
	{
		pSelectedNode = &node;
	}
	// signal if children should also be recursed
	return expanded;
}

void ComponentWindow::PopNode(ModelPart& node)
{
	ImGui::TreePop();
}

ComponentWindow::TransformParameters& ComponentWindow::resolveTransform() noexcept
{
	uint32_t id = pSelectedNode->GetId();
	auto i = transformParams.find(id);
	if (i == transformParams.end())
	{
		return loadTransform(id);
	}
	return i->second;
}

ComponentWindow::TransformParameters& ComponentWindow::loadTransform(uint32_t ID)
{
	const DirectX::XMFLOAT4X4& applied = pSelectedNode->GetAppliedTransform();
	const auto angles = ExtractEulerAngles(applied);
	const auto translation = ExtractTranslation(applied);
	TransformParameters tp;
	tp.zRot = angles.z;
	tp.xRot = angles.x;
	tp.yRot = angles.y;
	tp.x = translation.x;
	tp.y = translation.y;
	tp.z = translation.z;
	return transformParams.insert({ ID, { tp } }).first->second;
}