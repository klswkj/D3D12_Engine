#include "stdafx.h"
#include "ModelPart.h"
#include "ModelComponentWindow.h"

ModelPart::ModelPart(uint32_t ID, std::string Name, std::vector<Mesh*> pMeshes, const Math::Matrix4& InputMatrix) DEBUG_EXCEPT
	: m_ID(ID), m_pMeshes(std::move(pMeshes)), m_Name(Name)
{
	m_Transform = InputMatrix;
	m_AppliedTransform = Math::Matrix4(Math::EIdentityTag::kIdentity);
}

void ModelPart::Submit(eObjectFilterFlag Filter, Math::Matrix4 AccumulatedTransformMatrix) const DEBUG_EXCEPT
{
	const Math::Matrix4 built = m_AppliedTransform * m_Transform * AccumulatedTransformMatrix;

	for (const auto pm : m_pMeshes)
	{
		pm->Submit(Filter, built);
	}
	for (const auto& pc : m_ModelParts)
	{
		pc->Submit(Filter, built);
	}
}

void ModelPart::AddModelPart(std::unique_ptr<ModelPart> pChild) DEBUG_EXCEPT
{
	ASSERT(pChild);
	m_ModelParts.push_back(std::move(pChild));
}

void ModelPart::SetAppliedTransform(const Math::Matrix4 _Transform) noexcept
{
	m_AppliedTransform = _Transform;
}

void ModelPart::RecursivePushComponent(ModelComponentWindow& _ModelWindow)
{
	if (_ModelWindow.PushNode(*this)) // If childNode is exist.
	{
		for (auto& cp : m_ModelParts)
		{
			cp->RecursivePushComponent(_ModelWindow);
		}
		_ModelWindow.PopNode(*this);
	}
}

void ModelPart::PushAddon(IWindow& _TechniqueWindow)
{
	for (auto& mp : m_pMeshes)
	{
		mp->Accept(_TechniqueWindow);
	}
}
