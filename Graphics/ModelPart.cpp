#include "stdafx.h"
#include "ModelPart.h"
#include "ModelComponentWindow.h"
#include "TechniqueWindow.h"

ModelPart::ModelPart(uint32_t ID, std::string Name, std::vector<Mesh*> pMeshes, const DirectX::XMMATRIX& InputMatrix) DEBUG_EXCEPT
	: m_ID(ID), meshPtrs(std::move(pMeshes)), m_Name(Name)
{
	DirectX::XMStoreFloat4x4(&m_Transform, InputMatrix);
	DirectX::XMStoreFloat4x4(&m_AppliedTransform, DirectX::XMMatrixIdentity());
}

void ModelPart::Submit(eObjectFilterFlag Filter, DirectX::XMMATRIX AccumulatedTransformMatrix) const DEBUG_EXCEPT
{
	const DirectX::XMMATRIX built =
		( DirectX::XMLoadFloat4x4(&m_AppliedTransform) *
			DirectX::XMLoadFloat4x4(&m_Transform) *
			AccumulatedTransformMatrix );

	for (const auto pm : meshPtrs)
	{
		pm->Submit(Filter, built);
	}
	for (const auto& pc : childPtrs)
	{
		pc->Submit(Filter, built);
	}
}

void ModelPart::AddChild(std::unique_ptr<ModelPart> pChild) DEBUG_EXCEPT
{
	ASSERT(pChild);
	childPtrs.push_back(std::move(pChild));
}

void ModelPart::SetAppliedTransform(const DirectX::XMMATRIX _Transform) noexcept
{
	DirectX::XMStoreFloat4x4(&m_AppliedTransform, _Transform);
}

const DirectX::XMFLOAT4X4& ModelPart::GetAppliedTransform() const noexcept
{
	return m_AppliedTransform;
}

uint32_t ModelPart::GetId() const noexcept
{
	return m_ID;
}

void ModelPart::RecursivePushComponent(ModelComponentWindow& _ModelWindow)
{
	if (_ModelWindow.PushNode(*this)) // If childNode is exist.
	{
		for (auto& cp : childPtrs)
		{
			cp->RecursivePushComponent(_ModelWindow);
		}
		_ModelWindow.PopNode(*this);
	}
}

void ModelPart::PushAddon(ITechniqueWindow& _TechniqueWindow)
{
	for (auto& mp : meshPtrs)
	{
		mp->Accept(_TechniqueWindow);
	}
}
