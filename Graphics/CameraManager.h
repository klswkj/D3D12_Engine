#pragma once

namespace custom
{
	class CommandContext;
    class GraphicsContext;
}

class Camera;
class MasterRenderGraph;
enum class eObjectFilterFlag;

class CameraManager // CameraContainer, CameraController
{
public:
    struct CaptureBox;
public:
    CameraManager();
    void RenderWindows();
    void Bind(custom::GraphicsContext& BaseContext);
    void AddCamera(std::shared_ptr<Camera> pCam);
    void LinkTechniques(MasterRenderGraph& rg);
    void Submit(eObjectFilterFlag Filter) const;
    std::shared_ptr<Camera> GetActiveCamera() { return m_Cameras[m_ActiveCameraIndex]; }

    void Update(float deltaTime);
    void ToggleMomentum()         { m_Momentum = !m_Momentum; }
    void ToggleSlowMoveMent()     { m_bSlowMovement = !m_bSlowMovement; }
    void ToggleSlowRotation()     { m_bSlowRotation = !m_bSlowRotation; }

    void SetUpCamera(Camera* pCamera);
    void SetCaptureBox(void* AABB)        { m_CaptureBox = *static_cast<CaptureBox*>(AABB); }
    void SetCurrentHeading(float heading) { m_CurrentYaw = heading; }
    void SetCurrentPitch(float pitch)     { m_CurrentPitch = pitch; }

    Math::Vector3 GetWorldEast()  const { return m_WorldEast; }
    Math::Vector3 GetWorldUp()    const { return m_WorldUp; }
    Math::Vector3 GetWorldNorth() const { return m_WorldNorth; }
    float GetCurrentHeading()     const { return m_CurrentYaw; }
    float GetCurrentPitch()       const { return m_CurrentPitch; }
    CaptureBox GetCaputreBox()    const { return m_CaptureBox; }
    std::shared_ptr<Camera> GetControlledCamera() const { return m_Cameras[m_ControlledCameraIndex]; }

    bool CaptureCamera(const Math::Vector3& CameraPosition);

    struct CaptureBox
    {
        Math::Vector3 MinPoint = Math::Scalar(FLT_MAX);
        Math::Vector3 MaxPoint = Math::Scalar(FLT_MIN);
    };

private:
    void ApplyMomentum(float& oldValue, float& newValue, float deltaTime);

private:
	std::vector<std::shared_ptr<Camera>> m_Cameras;
    
	size_t m_ActiveCameraIndex     = 0;
	size_t m_ControlledCameraIndex = 0;
    size_t m_NumCamera             = 0;

    Math::Vector3 m_WorldUp;
    Math::Vector3 m_WorldNorth;
    Math::Vector3 m_WorldEast;
    
    CaptureBox m_CaptureBox;

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

    bool m_bSlowMovement;
    bool m_bSlowRotation;
    bool m_Momentum;

    static std::mutex sm_Mutex;
};