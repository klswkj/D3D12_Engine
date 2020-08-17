#include "stdafx.h"
#include "CameraManager.h"
#include "Window.h"
#include "CommandContext.h"
#include "MathBasic.h"
#include "Camera.h"
#include "Graphics.h"
#include "ObjectFilterFlag.h"
#include "MasterRenderGraph.h"
#include "WindowInput.h"

std::mutex CameraManager::sm_Mutex;

// 0. Draw Scene
// 1. CameraManager.RenderWindows() [ To Select Activated Camera ]
// 2. CameraManager => CameraManager::m_Cameras[m_ActiveCameraIndex].RenderWindow [ Controll Camera Component by manual ]
// 3. CameraManager.Update() => m_Cameras[m_ActiveCameraIndex]->Update()
// 4. Draw Imgui
// 5. Clear SceneBuffer


CameraManager::CameraManager()
{
	// Set { Up, North, East } <=> { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }
	m_WorldUp = Math::Normalize(Math::Vector3(Math::EYUnitVector::kYUnitVector));
	m_WorldNorth = -Normalize(Cross(m_WorldUp, Math::Vector3(Math::EXUnitVector::kXUnitVector)));
	m_WorldEast = Cross(m_WorldNorth, m_WorldUp);

	m_HorizontalLookSensitivity = 2.0f;
	m_VerticalLookSensitivity = 2.0f;
	m_MoveSpeed = 1000.0f;
	m_StrafeSpeed = 1000.0f;
	m_MouseSensitivityX = 1.0f;
	m_MouseSensitivityY = 1.0f;

	m_Cameras.reserve(20);

	// m_bSlowMovement = false;
	// m_bSlowRotation = false;
	// m_Momentum = true;
	/*
	m_CurrentPitch = 0.0f;
	m_CurrentYaw = 0.0f;
	// Must be init Pitch, forward, Heading
	// m_CurrentPitch = Math::Sin(Math::Dot(camera.GetForwardVec(), m_WorldUp));
	// Vector3 forward = Normalize(Cross(m_WorldUp, camera.GetRightVec()));
	// m_CurrentYaw = ATan2(-Dot(forward, m_WorldEast), Dot(forward, m_WorldNorth));
	m_LastYaw = 0.0f;
	m_LastPitch = 0.0f;
	m_LastForward = 0.0f;
	m_LastStrafe = 0.0f;
	m_LastAscent = 0.0f;
	*/
}

void CameraManager::SetUpCamera(Camera* pCamera)
{
	if (pCamera == nullptr)
	{
		ASSERT(false);
	}

	//    Math::Sin(Math::Dot(camera.GetForwardVec(), m_WorldUp)); 
	// => Math::Sin(Math::Dot(camera.m_basis.GetZ(), m_WorldUp))
	m_CurrentPitch = pCamera->GetPitch(); 
	
	// Math::Vector3 Forward = Math::Normalize(Math::Cross(m_WorldUp, camera.GetRightVec()));
	// m_CurrentYaw = ATan2(-Dot(forward, m_WorldEast), Dot(forward, m_WorldNorth))
	m_CurrentYaw = pCamera->GetYaw(); 

	m_LastYaw = 0.0f;
	m_LastPitch = 0.0f;
	m_LastForward = 0.0f;
	m_LastStrafe = 0.0f;
	m_LastAscent = 0.0f;
}

void CameraManager::Update(float deltaTime)
{
	Camera& TargetCamera = *m_Cameras[m_ActiveCameraIndex];

	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kLThumbClick) ||
		windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_lshift))
	{
		m_bSlowMovement = !m_bSlowMovement;
	}

	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kRThumbClick))
	{
		m_bSlowRotation = !m_bSlowRotation;
	}

	const float SpeedScale = (m_bSlowMovement) ? 0.5f : 1.0f;
	const float RotationScale = (m_bSlowRotation) ? 0.5f : 1.0f;

	float yaw = windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogRightStickX) *
		m_HorizontalLookSensitivity * RotationScale;

	float pitch = windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogRightStickY) *
		m_VerticalLookSensitivity * RotationScale;

	float forward = m_MoveSpeed * SpeedScale *
		(
			windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogLeftStickY) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_w) ? deltaTime : 0.0f) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_s) ? -deltaTime : 0.0f)
			);
	float strafe = m_StrafeSpeed * SpeedScale *
		(
			windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogLeftStickX) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_d) ? deltaTime : 0.0f) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_a) ? -deltaTime : 0.0f)
			);
	float ascent = m_StrafeSpeed * SpeedScale *
		(
			windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogRightTrigger) -
			windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogLeftTrigger) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_e) ? deltaTime : 0.0f) + // Up
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_q) ? -deltaTime : 0.0f) // Down
			);

	if (m_Momentum)
	{
		ApplyMomentum(m_LastYaw, yaw, deltaTime);
		ApplyMomentum(m_LastPitch, pitch, deltaTime);
		ApplyMomentum(m_LastForward, forward, deltaTime);
		ApplyMomentum(m_LastStrafe, strafe, deltaTime);
		ApplyMomentum(m_LastAscent, ascent, deltaTime);
	}

	yaw += windowInput::GetAnalogInput(windowInput::AnalogInput::kAnalogMouseX) * m_MouseSensitivityX;
	pitch += windowInput::GetAnalogInput(windowInput::AnalogInput::kAnalogMouseY) * m_MouseSensitivityY;

	m_CurrentPitch += pitch;
	m_CurrentPitch = DirectX::XMMin(DirectX::XM_PIDIV2, m_CurrentPitch);
	m_CurrentPitch = DirectX::XMMax(-DirectX::XM_PIDIV2, m_CurrentPitch);

	m_CurrentYaw -= yaw;
	if (DirectX::XM_PI < m_CurrentYaw)
	{
		m_CurrentYaw -= DirectX::XM_2PI;
	}
	else if (m_CurrentYaw <= -DirectX::XM_PI)
	{
		m_CurrentYaw += DirectX::XM_2PI;
	}

	TargetCamera.SetPitch(m_CurrentPitch);
	TargetCamera.SetYaw(m_CurrentYaw);

	// Orientation 방향
	// X : Right
	// Y : Up
	// Z : Forward
	// Position 위치
	Math::Matrix3 orientation = Math::Matrix3(m_WorldEast, m_WorldUp, -m_WorldNorth) * 
		Math::Matrix3::MakeYRotation(m_CurrentYaw) *   // -yaw
		Math::Matrix3::MakeXRotation(m_CurrentPitch);  // +Pitch
	// Math::Vector3 position = orientation * Math::Vector3(strafe, ascent, -forward) + m_TargetCamera.GetPosition();
	Math::Vector3 position = orientation * Math::Vector3(strafe, ascent, -forward) + TargetCamera.GetPosition();
	const DirectX::XMMATRIX CameraWorldMatrix = DirectX::XMMatrixLookToLH(DirectX::XMVECTOR(position), DirectX::XMVECTOR(orientation.GetZ()), DirectX::XMVECTOR(orientation.GetY()));

	// Original Code : m_TargetCamera.SetTransform( Math::AffineTransform( orientation, position ) );
	//m_TargetCamera.SetTransform(Math::OrthogonalTransform(Math::Quaternion(orientation), position));
	//m_TargetCamera.Update();
	TargetCamera.SetTransform(Math::OrthogonalTransform(Math::Quaternion(orientation), position));
	TargetCamera.Update();
}

void CameraManager::ApplyMomentum(float& oldValue, float& newValue, float deltaTime)
{
	float blendedValue;
	if (Math::Abs(oldValue) < Math::Abs(newValue))
	{
		blendedValue = Math::Lerp(newValue, oldValue, Math::Pow(0.6f, deltaTime * 60.0f));
	}
	else
	{
		blendedValue = Math::Lerp(newValue, oldValue, Math::Pow(0.8f, deltaTime * 60.0f));
	}
	oldValue = blendedValue;
	newValue = blendedValue;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

// Update -> Present -> Draw -> RenderWindows -> Update

void CameraManager::RenderWindows()
{
	if (ImGui::Begin("Cameras"))
	{
		std::shared_ptr<Camera> ActivatingCamera = GetActiveCamera();
		if (ImGui::BeginCombo("Active Camera", GetActiveCamera()->GetName()))
		{
			for (size_t CameraIndex = 0; CameraIndex < m_Cameras.size(); ++CameraIndex)
			{
				const bool bSelected = (CameraIndex == m_ActiveCameraIndex);

				if (ImGui::Selectable(m_Cameras[CameraIndex]->GetName(), bSelected))
				{
					m_ActiveCameraIndex = CameraIndex;
					// -> Set Position, Yaw, Pitch with ActiveCamera.
					SetUpCamera(m_Cameras[m_ActiveCameraIndex].get());
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::BeginCombo("Controlled Camera", GetControlledCamera()->GetName()))
		{
			for (size_t CameraIndex = 0; CameraIndex < m_Cameras.size(); ++CameraIndex)
			{
				const bool isSelected = CameraIndex == m_ControlledCameraIndex;
				if (ImGui::Selectable(m_Cameras[CameraIndex]->GetName(), isSelected))
				{
					m_ControlledCameraIndex = CameraIndex;
				}
			}
			ImGui::EndCombo();
		}

		GetControlledCamera()->RenderWindow(); // Camera, Update
	}
	ImGui::End();
}

void CameraManager::Bind(custom::CommandContext& BaseContext)
{
	// gfx.SetCamera((*this)->GetPerspectiveMatrix());
	// CommandContext에 CameraManager 추가.
}

void CameraManager::AddCamera(std::shared_ptr<Camera> pCam)
{
	m_Cameras.push_back(std::move(pCam));
}

void CameraManager::LinkTechniques(MasterRenderGraph& rg)
{
	for (auto& Camera : m_Cameras)
	{
		// Camera->LinkTechniques(rg);
	}
}

void CameraManager::Submit(eObjectFilterFlag Filter) const
{
	for (size_t CameraIndex = 0; CameraIndex < m_Cameras.size(); ++CameraIndex)
	{
		if (CameraIndex != m_ActiveCameraIndex)
		{
			m_Cameras[CameraIndex]->Submit(Filter);
		}
	}
}

std::shared_ptr<Camera> CameraManager::GetActiveCamera()
{
	return m_Cameras[m_ActiveCameraIndex];
}

std::shared_ptr<Camera> CameraManager::GetControlledCamera()
{
	return m_Cameras[m_ControlledCameraIndex];
}
