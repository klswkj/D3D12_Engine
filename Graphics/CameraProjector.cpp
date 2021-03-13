#include "stdafx.h"
#include "CameraProjector.h"
#include "CameraFrustum.h"
#include "Camera.h"
#include "ObjectFilterFlag.h"

CameraProjector::CameraProjector(Camera& rParent, const float VerticalFOV, const float AspectHeightOverWidth, const float NearZClip, const float FarZClip)
	:
	m_pParentCamera(&rParent),
	m_aspectHeightOverWidth(AspectHeightOverWidth),
	m_width(1.0f), m_height(1.0f / m_aspectHeightOverWidth),
	m_nearZClip(NearZClip), m_farZClip(FarZClip),
	m_verticalFOV(VerticalFOV),
	m_CameraFrustum(rParent, AspectHeightOverWidth, NearZClip, FarZClip),
	m_initAspect(AspectHeightOverWidth), m_initNearZClip(NearZClip), m_initFarZClip(FarZClip)
{
}

DirectX::XMMATRIX CameraProjector::GetProjectionMatrix() const
{
	return m_pParentCamera->GetProjMatrix();
	return DirectX::XMMatrixPerspectiveLH(m_width, m_height, m_nearZClip, m_farZClip);
}

void CameraProjector::RenderWidgets()
{
	bool bDirty = false;
	const auto dcheck = [&bDirty](bool d) { bDirty = bDirty || d; };

	ImGui::Text("Camera Projector");
	dcheck(ImGui::SliderAngle("VerticalFOV", &m_verticalFOV, -180.0f, 180.0f));
	dcheck(ImGui::SliderFloat("Width", &m_width, 0.01f, 4.0f, "%.2f", 1.5f));
	dcheck(ImGui::SliderFloat("Height", &m_height, 0.01f, 4.0f, "%.2f", 1.5f));
	dcheck(ImGui::SliderFloat("Near Z", &m_nearZClip , 0.01f, m_farZClip - 0.01f, "%.2f", 4.0f));
	dcheck(ImGui::SliderFloat("Far Z", &m_farZClip, m_nearZClip + 0.01f, 400.0f, "%.2f", 4.0f));

	if (bDirty)
	{
		m_CameraFrustum.SetVertices(m_width, m_height, m_nearZClip, m_farZClip);
		m_pParentCamera->SetPerspectiveMatrix(m_verticalFOV, m_width / m_height, m_nearZClip, m_farZClip);
	}
}

void CameraProjector::Submit(const eObjectFilterFlag filter)
{
	m_CameraFrustum.Submit(filter);
}

void CameraProjector::LinkTechniques(const MasterRenderGraph& masterRG)
{
	m_CameraFrustum.LinkTechniques(masterRG);
}

void CameraProjector::Reset()
{
	m_width = 1.0f;
	m_height = 1.0f / m_initAspect;
	m_nearZClip = m_initNearZClip;
	m_farZClip = m_initFarZClip;
	m_CameraFrustum.SetVertices(m_initAspect, m_nearZClip, m_farZClip);
}
