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
    D3D11 void RenderWindows();
    D3D11 void Bind(custom::CommandContext& BaseContext);
    D3D11 void AddCamera(std::shared_ptr<Camera> pCam);
    D3D11 void LinkTechniques(MasterRenderGraph& rg);
    D3D11 void Submit(eObjectFilterFlag Filter) const;
    D3D11 std::shared_ptr<Camera> GetActiveCamera();

    D3D12 void Update(float deltaTime);
    D3D12 void ToggleSlowMoveMent() { m_bSlowMovement = !m_bSlowMovement; }
    D3D12 void ToggleSlowRotation() { m_bSlowRotation = !m_bSlowRotation; }
    D3D12 Math::Vector3 GetWorldEast() { return m_WorldEast; }
    D3D12 Math::Vector3 GetWorldUp() { return m_WorldUp; }
    D3D12 Math::Vector3 GetWorldNorth() { return m_WorldNorth; }
    D3D12 float GetCurrentHeading() { return m_CurrentYaw; }
    D3D12 float GetCurrentPitch() { return m_CurrentPitch; }

    D3D12 void SetCurrentHeading(float heading) { m_CurrentYaw = heading; }
    D3D12 void SetCurrentPitch(float pitch) { m_CurrentPitch = pitch; }

    void SetUpCamera(Camera* pCamera);

private:
    D3D11 std::shared_ptr<Camera> GetControlledCamera();
    D3D12 void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);
    void operator= (Camera _Camera) { m_Cameras.push_back(std::make_shared<Camera>(_Camera)); ++m_NumCamera; }
    // void CameraPushBack(Camera Camera);

private:
	std::vector<std::shared_ptr<Camera>> m_Cameras;
    static std::mutex sm_Mutex;

	size_t m_ActiveCameraIndex{ 0 };
	size_t m_ControlledCameraIndex{ 0 };
    size_t m_NumCamera{ 0 };

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