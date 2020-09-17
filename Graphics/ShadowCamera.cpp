#include "stdafx.h"
#include "ShadowCamera.h"
#include "CommandContext.h"

void ShadowCamera::BindToGraphics(custom::CommandContext& BaseContext)
{
    custom::GraphicsContext& graphicsContext = BaseContext.GetGraphicsContext();

    graphicsContext.SetModelToProjectionByCamera(*this);
}

void ShadowCamera::UpdateShadowMatrix
(
    Math::Vector3 LightDirection, Math::Vector3 ShadowCenter, Math::Vector3 ShadowBounds,
    uint32_t BufferWidth, uint32_t BufferHeight, uint32_t BufferPrecision
)
{
    SetLookDirection(LightDirection, Math::Vector3(Math::EZUnitVector::kZUnitVector));

    // Converts world units to texel units so we can quantize the camera position to whole texel units
    Math::Vector3 RcpDimensions = Recip(ShadowBounds);
    Math::Vector3 QuantizeScale = Math::Vector3((float)BufferWidth, (float)BufferHeight, (float)((1 << BufferPrecision) - 1)) * RcpDimensions;

    //// Recenter the camera at the quantized position
    // Transform to view space
    ShadowCenter = ~GetRotation() * ShadowCenter;
    // Scale to texel units, truncate fractional part, and scale back to world units
    ShadowCenter = Floor(ShadowCenter * QuantizeScale) / QuantizeScale;
    // Transform back into world space
    ShadowCenter = GetRotation() * ShadowCenter;

    SetPosition(ShadowCenter);

    SetProjMatrix(Math::Matrix4::MakeScale(Math::Vector3(2.0f, 2.0f, 1.0f) * RcpDimensions));

    Update();

    // Transform from clip space to texture space
    m_ShadowMatrix = Math::Matrix4(Math::AffineTransform(Math::Matrix3::MakeScale(0.5f, -0.5f, 1.0f), Math::Vector3(0.5f, 0.5f, 0.0f))) * m_ViewProjMatrix;
}
