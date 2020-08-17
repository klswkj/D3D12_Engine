#pragma once
#include "stdafx.h"
#include "ObjectFilterFlag.h"
class Model;
class ComponentWindow;
class AddonWindow;
class Mesh;

class ModelPart
{
	friend class Model;
public:
	ModelPart(uint32_t ID, const char* name, std::vector<Mesh*> meshPtrs, const DirectX::XMMATRIX& InputMatrix) DEBUG_EXCEPT;
	void Submit(eObjectFilterFlag filter, const DirectX::XMMATRIX accumulatedTransform) const DEBUG_EXCEPT;
	void SetAppliedTransform(DirectX::XMMATRIX transform) noexcept;
	const DirectX::XMFLOAT4X4& GetAppliedTransform() const noexcept;
	uint32_t GetId() const noexcept;
	bool HasChildren() const noexcept;
	void RecursivePushComponent(ComponentWindow& probe);
	void PushAddon(AddonWindow& probe);
	const char* GetName() const;
private:
	void AddChild(std::unique_ptr<ModelPart> pChild) DEBUG_EXCEPT;

private:
	std::vector<std::unique_ptr<ModelPart>> childPtrs;
	std::vector<Mesh*> meshPtrs;

	const char* name;
	uint32_t id;
	DirectX::XMFLOAT4X4 transform;
	DirectX::XMFLOAT4X4 appliedTransform;
};

inline bool ModelPart::HasChildren() const noexcept { return 0 < childPtrs.size(); }
inline const char* ModelPart::GetName() const { return name; }