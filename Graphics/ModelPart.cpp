#include "stdafx.h"
#include "ModelPart.h"
#include "ModelComponentWindow.h"
#include "TechniqueWindow.h"

ModelPart::ModelPart(uint32_t ID, const char* name, std::vector<Mesh*> meshPtrs, const DirectX::XMMATRIX& InputMatrix) DEBUG_EXCEPT
	: id(ID), meshPtrs(std::move(meshPtrs)), name(name)
{
	DirectX::XMStoreFloat4x4(&transform, InputMatrix);
	DirectX::XMStoreFloat4x4(&appliedTransform, DirectX::XMMatrixIdentity());
}

void ModelPart::Submit(eObjectFilterFlag filter, DirectX::XMMATRIX accumulatedTransform) const DEBUG_EXCEPT
{
	const DirectX::XMMATRIX built =
		( DirectX::XMLoadFloat4x4(&appliedTransform) *
			DirectX::XMLoadFloat4x4(&transform) *
			accumulatedTransform );

	for (const auto pm : meshPtrs)
	{
		pm->Submit(filter, built);
	}
	for (const auto& pc : childPtrs)
	{
		pc->Submit(filter, built);
	}
}

void ModelPart::AddChild(std::unique_ptr<ModelPart> pChild) DEBUG_EXCEPT
{
	ASSERT(pChild);
	childPtrs.push_back(std::move(pChild));
}

void ModelPart::SetAppliedTransform(const DirectX::XMMATRIX transform) noexcept
{
	DirectX::XMStoreFloat4x4(&appliedTransform, transform);
}

const DirectX::XMFLOAT4X4& ModelPart::GetAppliedTransform() const noexcept
{
	return appliedTransform;
}

uint32_t ModelPart::GetId() const noexcept
{
	return id;
}

void ModelPart::RecursivePushComponent(ModelComponentWindow& _Window)
{
	if (_Window.PushNode(*this)) // If childNode is exist.
	{
		for (auto& cp : childPtrs)
		{
			cp->RecursivePushComponent(_Window);
		}
		_Window.PopNode(*this);
	}
}

void ModelPart::PushAddon(ITechniqueWindow& probe)
{
	for (auto& mp : meshPtrs)
	{
		mp->Accept(probe);
	}
}
