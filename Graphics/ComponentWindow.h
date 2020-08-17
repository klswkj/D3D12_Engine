#pragma once
#include "stdafx.h"
// class ModelPart;
#include "ModelPart.h"

class ComponentWindow
{
public:
	ComponentWindow(std::string name)
		: name(std::move(name))
	{
	}
	void SpawnWindow(Model& model);

	bool PushNode(ModelPart& node);
	void PopNode(ModelPart& node)
	{
		ImGui::TreePop();
	}
	struct TransformParameters
	{
		float xRot = 0.0f;
		float yRot = 0.0f;
		float zRot = 0.0f;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};
private:
	TransformParameters& resolveTransform();
	TransformParameters& loadTransform(uint32_t ID);
private:
	ModelPart* pSelectedNode = nullptr;

	std::string name;
	std::unordered_map<uint32_t, TransformParameters> transformParams;
};