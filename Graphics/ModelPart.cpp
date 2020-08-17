#include "stdafx.h"
#include "ModelPart.h"
#include "Mesh.h"
#include "ComponentWindow.h"
// #include "AddonWindow.h"

ModelPart::ModelPart(uint32_t ID, const char* name, std::vector<Mesh*> meshPtrs, const DirectX::XMMATRIX& InputMatrix) DEBUG_EXCEPT
	: id(ID), meshPtrs(std::move(meshPtrs)), name(name)
{
	DirectX::XMStoreFloat4x4(&transform, InputMatrix);
	DirectX::XMStoreFloat4x4(&appliedTransform, DirectX::XMMatrixIdentity());
}

void ModelPart::Submit(eObjectFilterFlag filter, const DirectX::XMMATRIX accumulatedTransform) const DEBUG_EXCEPT
{
	const auto built = ( 
		DirectX::XMLoadFloat4x4(&appliedTransform) *
		DirectX::XMLoadFloat4x4(&transform) * accumulatedTransform
			);

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

void ModelPart::RecursivePushComponent(ComponentWindow& probe)
{
	if (probe.PushNode(*this)) // If childNode is exist.
	{
		for (auto& cp : childPtrs)
		{
			cp->RecursivePushComponent(probe);
		}
		probe.PopNode(*this);
	}
}

void ModelPart::PushAddon(AddonWindow& probe)
{
	for (auto& mp : meshPtrs)
	{
		mp->Accept(probe);
	}
}
