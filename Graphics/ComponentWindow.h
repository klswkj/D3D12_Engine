#pragma once
#include "ModelPart.h"
class Model;

class ComponentWindow
{
public:
	ComponentWindow(std::string name)
		: name(std::move(name))
	{
	}
	void ShowWindow(Model& model);

	bool PushNode(ModelPart& node);
	void PopNode(ModelPart& node);
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
	TransformParameters& resolveTransform() noexcept;
	TransformParameters& loadTransform(uint32_t ID);

private:
	ModelPart* pSelectedNode = nullptr;

	std::string name;
	std::unordered_map<uint32_t, TransformParameters> transformParams;
};