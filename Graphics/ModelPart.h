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
	ModelPart(uint32_t _ID, std::string _Name, std::vector<Mesh*> pMeshes, const Math::Matrix4& InputMatrix) DEBUG_EXCEPT;
	void Submit(eObjectFilterFlag _FilterFlag, Math::Matrix4 AccumulatedTransformMatrix) const DEBUG_EXCEPT;
	void SetAppliedTransform(Math::Matrix4 _Transform) noexcept;

	Math::Matrix4& GetAppliedTransform() noexcept { return m_AppliedTransform; }
	std::string GetName() const { return m_Name; }
	uint32_t GetKey() const noexcept { return m_ID; }

	bool HasChildren() const noexcept { return 0 < m_ModelParts.size(); }
	void RecursivePushComponent(ModelComponentWindow& _ModelWindow);
	void PushAddon(IWindow& _TechniqueWindow);

private:
	void AddModelPart(std::unique_ptr<ModelPart> pModelPart) DEBUG_EXCEPT;

private:
	std::string m_Name;
	uint32_t m_ID;
	// DirectX::XMFLOAT4X4 m_Transform;
	// DirectX::XMFLOAT4X4 m_AppliedTransform;
	Math::Matrix4 m_Transform;
	Math::Matrix4 m_AppliedTransform;

	std::vector<std::unique_ptr<ModelPart>> m_ModelParts;
	std::vector<Mesh*> m_pMeshes;
};