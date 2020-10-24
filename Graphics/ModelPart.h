#pragma once
#include "stdafx.h"
#include "ObjectFilterFlag.h"
#include "Mesh.h"

class Model;
class ModelComponentWindow;
class ITechniqueWindow;

class ModelPart
{
	friend class Model;
public:
	ModelPart(uint32_t _ID, std::string _Name, std::vector<Mesh*> pMeshes, const DirectX::XMMATRIX& InputMatrix) DEBUG_EXCEPT;
	void Submit(eObjectFilterFlag _FilterFlag, DirectX::XMMATRIX AccumulatedTransformMatrix) const DEBUG_EXCEPT;
	void SetAppliedTransform(DirectX::XMMATRIX _Transform) noexcept;
	const DirectX::XMFLOAT4X4& GetAppliedTransform() const noexcept;

	uint32_t GetId() const noexcept;
	bool HasChildren() const noexcept;
	void RecursivePushComponent(ModelComponentWindow& _ModelWindow);
	void PushAddon(ITechniqueWindow& _TechniqueWindow);
	std::string GetName() const;
private:
	void AddChild(std::unique_ptr<ModelPart> pChild) DEBUG_EXCEPT;

private:
	std::vector<std::unique_ptr<ModelPart>> childPtrs;
	std::vector<Mesh*> meshPtrs;

	std::string m_Name;
	uint32_t m_ID;
	DirectX::XMFLOAT4X4 m_Transform;
	DirectX::XMFLOAT4X4 m_AppliedTransform;
};

inline bool ModelPart::HasChildren() const noexcept { return 0 < childPtrs.size(); }
inline std::string ModelPart::GetName() const { return m_Name; }