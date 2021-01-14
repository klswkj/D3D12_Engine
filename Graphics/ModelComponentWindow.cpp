#include "stdafx.h"
#include "ModelComponentWindow.h"
#include "Model.h"
// #include "ModelPart.h"

void ModelComponentWindow::ShowWindow(Model& _Model) // ModelProbe
{
	ImGui::Columns(2, nullptr, true);
	// ImGui::BeginChild("Model Tree", ImVec2(0, 200), ImGuiWindowFlags_AlwaysVerticalScrollbar);
	ImGui::BeginChild("Model Tree", ImVec2(0, 200));
	_Model.TraverseNode(*this);
	ImGui::EndChild(); // End child of Model Tree

	if (m_pSelectedModelPart != nullptr)
	{
		// ImGui::NextColumn();
		ImGui::BeginChild("Model Component");

		bool bDirty = false;
		const auto dcheck = [&bDirty](bool changed) { bDirty = bDirty || changed; };

		ModelComponentWindow::TransformParameters& tf = resolveTransform();
		ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Translation");
		dcheck(ImGui::SliderFloat("X", &tf.x, -60.f, 60.f));
		dcheck(ImGui::SliderFloat("Y", &tf.y, -60.f, 60.f));
		dcheck(ImGui::SliderFloat("Z", &tf.z, -60.f, 60.f));
		ImGui::TextColored({ 0.4f,1.0f,0.6f,1.0f }, "Orientation");
		dcheck(ImGui::SliderAngle("X-rotation", &tf.xRot, -180.0f, 180.0f));
		dcheck(ImGui::SliderAngle("Y-rotation", &tf.yRot, -180.0f, 180.0f));
		dcheck(ImGui::SliderAngle("Z-rotation", &tf.zRot, -180.0f, 180.0f));

		if (bDirty)
		{
			DirectX::XMMATRIX xRot = DirectX::XMMatrixRotationX(tf.xRot);
			DirectX::XMMATRIX yRot = DirectX::XMMatrixRotationY(tf.yRot);
			DirectX::XMMATRIX zRot = DirectX::XMMatrixRotationZ(tf.zRot);
			DirectX::XMMATRIX xyz  = DirectX::XMMatrixTranslation(tf.x, tf.y, tf.z);

			Math::Matrix4 AppliedMatrix = Math::Matrix4(xRot * yRot * zRot * xyz);

			m_pSelectedModelPart->SetAppliedTransform(AppliedMatrix);
		}

		ImGui::EndChild(); // End child of Model Component
	    ImGui::NextColumn();
		ImGui::BeginChild("Material Child");
		MaterialWindow addon;
		m_pSelectedModelPart->PushAddon(addon);
		ImGui::EndChild();
	}
}

bool ModelComponentWindow::PushNode(ModelPart& _ModelPart)
{
	// If there is no selected ModelPart, set selectedId to an impossible value
	bool           bHasChildren = _ModelPart.HasChildren();
	const uint32_t selectedId   = (m_pSelectedModelPart == nullptr) ? -1 : m_pSelectedModelPart->GetKey();
	uint32_t       NodeID       = _ModelPart.GetKey();

	// Build up flags for current _ModelPart
	const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | // Only open when clicking on the arrow part. If ImGuiTreeNodeFlags_OpenOnDoubleClick is also set, single-click arrow or double-click all box to open.
		ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_FramePadding      |
		((NodeID == selectedId) ? ImGuiTreeNodeFlags_Selected : 0)       | // Draw as selected
		(bHasChildren ? 0 : ImGuiTreeNodeFlags_Leaf);                      // No collapsing, no arrow (use as a convenience for leaf nodes).

	// render this _ModelPart
	const auto expanded = ImGui::TreeNodeEx
	(
		(void*)&NodeID, 
		node_flags, 
		_ModelPart.GetName().c_str()
	);

	// processing for selecting ModelPart
	if (ImGui::IsItemClicked())
	{
		m_pSelectedModelPart = &_ModelPart;
	}
	// signal if children should also be recursed
	return expanded;
}

void ModelComponentWindow::PopNode(ModelPart&)
{
	ImGui::TreePop();
}

ModelComponentWindow::TransformParameters& ModelComponentWindow::resolveTransform() noexcept
{
	uint32_t _Key = m_pSelectedModelPart->GetKey();

	auto i = m_TransformParameter.find(_Key);

	if (i == m_TransformParameter.end())
	{
		return loadTransform(_Key);
	}
	return i->second;
}

ModelComponentWindow::TransformParameters& ModelComponentWindow::loadTransform(uint32_t _Key)
{
	// DirectX::XMFLOAT4X4& applied = DirectX::XMFLOAT4X4(m_pSelectedModelPart->GetAppliedTransform());
	// DirectX::XMFLOAT4X4 applied = m_pSelectedModelPart->GetAppliedTransform().GetXMFLOAT4X4();
	Math::Matrix4 NodeMatrix    = m_pSelectedModelPart->GetAppliedTransform();
	DirectX::XMFLOAT4X4 applied = NodeMatrix.GetXMFLOAT4X4();

	const DirectX::XMFLOAT3 angles      = ExtractEulerAngles(applied);
	const DirectX::XMFLOAT3 translation = ExtractTranslation(applied);

	TransformParameters tp;
	tp.zRot = angles.z;
	tp.xRot = angles.x;
	tp.yRot = angles.y;
	tp.x    = translation.x;
	tp.y    = translation.y;
	tp.z    = translation.z;

	return m_TransformParameter.insert({ _Key, { tp } }).first->second;
}