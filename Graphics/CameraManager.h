#pragma once
#include "MathBasic.h"

#define D3D11 
#define D3D12 

namespace custom
{
	class CommandContext;
}

class Camera;
class MasterRenderGraph;
enum class eObjectFilterFlag;

class CameraManager // CameraContainer, CameraController
{
public:
    CameraManager();
    void RenderWindows();
    void Bind(custom::CommandContext& BaseContext);
    void AddCamera(std::shared_ptr<Camera> pCam);
    void LinkTechniques(MasterRenderGraph& rg);
    void Submit(eObjectFilterFlag Filter) const;
    std::shared_ptr<Camera> GetActiveCamera();

    void Update(float deltaTime);
    void ToggleSlowMoveMent() { m_bSlowMovement = !m_bSlowMovement; }
    void ToggleSlowRotation() { m_bSlowRotation = !m_bSlowRotation; }
    Math::Vector3 GetWorldEast() { return m_WorldEast; }
    Math::Vector3 GetWorldUp() { return m_WorldUp; }
    Math::Vector3 GetWorldNorth() { return m_WorldNorth; }
    float GetCurrentHeading() { return m_CurrentYaw; }
    float GetCurrentPitch() { return m_CurrentPitch; }

    void SetCurrentHeading(float heading) { m_CurrentYaw = heading; }
    void SetCurrentPitch(float pitch) { m_CurrentPitch = pitch; }

    void SetUpCamera(Camera* pCamera);
    std::shared_ptr<Camera> GetControlledCamera();

private:
    void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

private:
	std::vector<std::shared_ptr<Camera>> m_Cameras;
    static std::mutex sm_Mutex;

	size_t m_ActiveCameraIndex= 0;
	size_t m_ControlledCameraIndex= 0;
    size_t m_NumCamera= 0;

    Math::Vector3 m_WorldUp;
    Math::Vector3 m_WorldNorth;
    Math::Vector3 m_WorldEast;

    float m_HorizontalLookSensitivity;
    float m_VerticalLookSensitivity;
    float m_MoveSpeed;
    float m_StrafeSpeed;
    float m_MouseSensitivityX;
    float m_MouseSensitivityY;

    float m_CurrentYaw;
    float m_CurrentPitch; // m_CurrentHeading.

    float m_LastYaw;
    float m_LastPitch;
    float m_LastForward;
    float m_LastStrafe;
    float m_LastAscent;

    bool m_bSlowMovement{ false };
    bool m_bSlowRotation{ false };
    bool m_Momentum{ true };
};


// static constexpr float m_MovementSpeed{ 12.0f }; -> Move to CameraManager.
// static constexpr float m_RotationSpeed{ 0.004f };