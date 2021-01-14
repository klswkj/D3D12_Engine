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
#include "CustomImgui.h"

std::mutex CameraManager::sm_Mutex;

// 0. Draw Scene
// 1. CameraManager.RenderWindows() [ To Select Activated Camera ]
// 2. CameraManager => CameraManager::m_Cameras[m_ActiveCameraIndex].RenderWindow [ Controll Camera Component by manual ]
// 3. CameraManager.Update() => m_Cameras[m_ActiveCameraIndex]->Update()
// 4. Draw Imgui
// 5. Clear SceneBuffer

CameraManager::CameraManager()
{
	m_Cameras.reserve(20);
	AddCamera(std::make_shared<Camera>(this, "InitCamera"));
	m_ActiveCameraIndex = 0;

	Camera& InitCamera = **m_Cameras.begin();
	// InitCamera.SetEyeAtUp(Math::Vector3(-13.5f, 6.0f, 3.5f), Math::Vector3(Math::EZeroTag::kZero), Math::Vector3(Math::EYUnitVector::kYUnitVector));
	InitCamera.SetEyeAtUp(Math::Vector3(1098.72424f, 651.495361f, -38.6905518f), Math::Vector3(Math::EZeroTag::kZero), Math::Vector3(Math::EYUnitVector::kYUnitVector));
	InitCamera.SetZRange(1.0f, 10000.0f);
	// Set { Up, North, East } <=> { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }
	m_WorldUp    = Math::Normalize(Math::Vector3(Math::EYUnitVector::kYUnitVector));
	m_WorldNorth = -Normalize(Cross(m_WorldUp, Math::Vector3(Math::EXUnitVector::kXUnitVector)));
	m_WorldEast  = Cross(m_WorldNorth, m_WorldUp);

	m_HorizontalLookSensitivity = 2.0f;
	m_VerticalLookSensitivity   = 2.0f;
	m_MoveSpeed                 = 1000.0f;
	m_StrafeSpeed               = 1000.0f;
	m_MouseSensitivityX         = 1.0f;
	m_MouseSensitivityY         = 1.0f;

	m_bSlowMovement = false;
	m_bSlowRotation = false;
	m_Momentum      = true;

	const Math::Vector3 temp = InitCamera.GetRightVec();

	Math::Vector3 forward = Normalize(Cross(m_WorldUp, temp));
	m_CurrentPitch        = Math::Sin(Math::Dot(InitCamera.GetForwardVec(), m_WorldUp));
	m_CurrentYaw          = Math::ATan2(-Math::Dot(forward, m_WorldEast), Dot(forward, m_WorldNorth));
	// Must be init Pitch, forward, Heading

	m_LastYaw     = 0.0f;
	m_LastPitch   = 0.0f;
	m_LastForward = 0.0f;
	m_LastStrafe  = 0.0f;
	m_LastAscent  = 0.0f;
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

	m_LastYaw     = 0.0f;
	m_LastPitch   = 0.0f;
	m_LastForward = 0.0f;
	m_LastStrafe  = 0.0f;
	m_LastAscent  = 0.0f;
}

void CameraManager::Update(float DeltaTime)
{
	Camera& TargetCamera = *m_Cameras[m_ActiveCameraIndex];

	if (!CaptureCamera(TargetCamera.GetPosition()))
	{
		float modelRadius = Math::Length(m_CaptureBox.MaxPoint - m_CaptureBox.MinPoint) * 0.5f;
		const Math::Vector3 cameraPosition = (m_CaptureBox.MaxPoint + m_CaptureBox.MinPoint) * 0.5f + Math::Vector3(modelRadius * 0.5f, 0.0f, 0.0f);
		TargetCamera.SetEyeAtUp(cameraPosition, Math::Vector3(Math::EZeroTag::kZero), Math::Vector3(Math::EYUnitVector::kYUnitVector)); // CameraPosition, AtVector, UpVector
	}

	if (windowInput::IsFirstPressed(windowInput::DigitalInput::kLThumbClick) ||
		windowInput::IsFirstPressed(windowInput::DigitalInput::kKey_lshift))
	{
		m_bSlowMovement = !m_bSlowMovement;
	}

	const float SpeedScale    = (m_bSlowMovement) ? 0.5f : 1.0f;
	const float RotationScale = (m_bSlowRotation) ? 0.5f : 1.0f;

	float yaw = windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogRightStickX) *
		m_HorizontalLookSensitivity * RotationScale;

	float pitch = windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogRightStickY) *
		m_VerticalLookSensitivity * RotationScale;

	float forward = m_MoveSpeed * SpeedScale *
		(
			windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogLeftStickY) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_w) ? DeltaTime : 0.0f) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_s) ? -DeltaTime : 0.0f)
			);

	float strafe = m_StrafeSpeed * SpeedScale *
		(
			windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogLeftStickX) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_d) ? DeltaTime : 0.0f) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_a) ? -DeltaTime : 0.0f)
			);
	float ascent = m_StrafeSpeed * SpeedScale *
		(
			windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogRightTrigger) -
			windowInput::GetTimeCorrectedAnalogInput(windowInput::AnalogInput::kAnalogLeftTrigger) +
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_e) ? DeltaTime : 0.0f) + // Up
			(windowInput::IsPressed(windowInput::DigitalInput::kKey_q) ? -DeltaTime : 0.0f) // Down
			);

	if (m_Momentum)
	{
		ApplyMomentum(m_LastYaw, yaw, DeltaTime);
		ApplyMomentum(m_LastPitch, pitch, DeltaTime);
		ApplyMomentum(m_LastForward, forward, DeltaTime);
		ApplyMomentum(m_LastStrafe, strafe, DeltaTime);
		ApplyMomentum(m_LastAscent, ascent, DeltaTime);
	}
	else
	{
		yaw += windowInput::GetAnalogInput(windowInput::AnalogInput::kAnalogMouseX) * m_MouseSensitivityX;
		pitch += windowInput::GetAnalogInput(windowInput::AnalogInput::kAnalogMouseY) * m_MouseSensitivityY;
	}

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

	// Orientation 
	// X : Right
	// Y : Up
	// Z : Forward
	// Position
	Math::Matrix3 orientation =
		Math::Matrix3(m_WorldEast, m_WorldUp, -m_WorldNorth) *
		Math::Matrix3::MakeYRotation(m_CurrentYaw) *   // -yaw
		Math::Matrix3::MakeXRotation(m_CurrentPitch);  // +Pitch

	// Math::Vector3 position = orientation * Math::Vector3(strafe, ascent, -forward) + m_TargetCamera.GetPosition();
	Math::Vector3 position = (orientation * Math::Vector3(strafe, ascent, -forward)) + TargetCamera.GetPosition();
	const DirectX::XMMATRIX CameraWorldMatrix = DirectX::XMMatrixLookToLH(DirectX::XMVECTOR(position), DirectX::XMVECTOR(orientation.GetZ()), DirectX::XMVECTOR(orientation.GetY()));
	/*
#ifdef _DEBUG
	static size_t m_CurrentIndex = 0;

	if (!((m_CurrentIndex++ - 2) % 100ull))
	{
		float xx = position.GetX();
		float yy = position.GetY();
		float zz = position.GetZ();

		float upX = TargetCamera.GetUpVec().GetX();
		float upY = TargetCamera.GetUpVec().GetY();
		float upZ = TargetCamera.GetUpVec().GetZ();

		float rightX = TargetCamera.GetRightVec().GetX();
		float rightY = TargetCamera.GetRightVec().GetY();
		float rightZ = TargetCamera.GetRightVec().GetZ();

		float yawyaw = TargetCamera.GetYaw();

		printf("\nCamera Position : %.5f, %.5f, %.5f \n", xx, yy, zz);
		printf("Camera Up : %.5f, %.5f, %.5f \n", upX, upY, upZ);
		printf("Camera Right : %.5f, %.5f, %.5f \n", rightX, rightY, rightZ);
		printf("Camera Yaw : %.5f\n\n\n", yawyaw);
		printf("Caputre Min : %.5f, %.5f, %.5f \n", float(m_CaptureBox.MinPoint.GetX()), float(m_CaptureBox.MinPoint.GetY()), float(m_CaptureBox.MinPoint.GetZ()));
		printf("Caputre Max : %.5f, %.5f, %.5f \n", float(m_CaptureBox.MaxPoint.GetX()), float(m_CaptureBox.MaxPoint.GetY()), float(m_CaptureBox.MaxPoint.GetZ()));
	}
#endif
	*/
	// Original Code : m_TargetCamera.SetTransform( Math::AffineTransform( orientation, position ) );
	//m_TargetCamera.SetTransform(Math::OrthogonalTransform(Math::Quaternion(orientation), position));
	//m_TargetCamera.Update();
	if (CaptureCamera(position))
	{
		TargetCamera.SetTransform(Math::OrthogonalTransform(Math::Quaternion(orientation), position));
		TargetCamera.Update();
	}
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
		ImGui::Columns(2, nullptr, true);
		ImGui::BeginChild("CameraManager");

		std::shared_ptr<Camera> ActivatingCamera = GetActiveCamera();

		ImGui::Text("Active Camera");
		if (ImGui::BeginCombo("", ActivatingCamera->GetName().c_str()))
		{
			for (size_t CameraIndex = 0; CameraIndex < m_Cameras.size(); ++CameraIndex)
			{
				const bool bSelected = (CameraIndex == m_ActiveCameraIndex);

				if (ImGui::Selectable(m_Cameras[CameraIndex]->GetName().c_str(), bSelected))
				{
					m_ActiveCameraIndex = CameraIndex;
					// -> Set Position, Yaw, Pitch with ActiveCamera.
					SetUpCamera(m_Cameras[m_ActiveCameraIndex].get());
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Text("Controlled Camera");
		if (ImGui::BeginCombo("", GetControlledCamera()->GetName().c_str()))
		{
			for (size_t CameraIndex = 0; CameraIndex < m_Cameras.size(); ++CameraIndex)
			{
				const bool isSelected = CameraIndex == m_ControlledCameraIndex;
				if (ImGui::Selectable(m_Cameras[CameraIndex]->GetName().c_str(), isSelected))
				{
					m_ControlledCameraIndex = CameraIndex;
				}
			}
			ImGui::EndCombo();
		}

		static char Characterbuffer[31];

		ImGui::Spacing();
		ImGui::Text("New Camera Name");
		ImGui::InputText("", Characterbuffer, IM_ARRAYSIZE(Characterbuffer));

		// 카메라 추가.
		if (ImGui::Button("Add Camera"))
		{
			Characterbuffer[30] = '\0';

			AddCamera(std::make_shared<Camera>(this, &Characterbuffer[0]));

			Characterbuffer[0] = '\0';
		}

		ImGui::EndChild();
		ImGui::NextColumn();
		ImGui::BeginChild("Controlled Camera");
		GetControlledCamera()->RenderWindow(); // Camera, Update
		ImGui::EndChild();
	}
	
	ImGui::End();
}

void CameraManager::Bind(custom::CommandContext& BaseContext)
{
	BaseContext.SetMainCamera(*GetControlledCamera());
}

void CameraManager::AddCamera(std::shared_ptr<Camera> pCam)
{
	m_Cameras.push_back(std::move(pCam));
}

void CameraManager::LinkTechniques(MasterRenderGraph& rg)
{
	for (auto& Camera : m_Cameras)
	{
		Camera->LinkTechniques(rg);
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

bool CameraManager::CaptureCamera(const Math::Vector3& CameraPosition)
{
	// ASSERT(m_CaptureBox.MinPoint.GetX() != Math::Scalar(FLT_MAX));
	// ASSERT(m_CaptureBox.MaxPoint.GetX() != Math::Scalar(FLT_MIN));

	{
		float CamX = CameraPosition.GetX();
		float CamY = CameraPosition.GetY();
		float CamZ = CameraPosition.GetZ();

		float MinX = m_CaptureBox.MinPoint.GetX();
		float MinY = m_CaptureBox.MinPoint.GetY();
		float MinZ = m_CaptureBox.MinPoint.GetZ();

		float MaxX = m_CaptureBox.MaxPoint.GetX();
		float MaxY = m_CaptureBox.MaxPoint.GetY();
		float MaxZ = m_CaptureBox.MaxPoint.GetZ();

		if ( (CamX < MinX) ||
			(MaxX < CamX) ||
			(CamY < MinY) ||
			(MaxY < CamY) ||
			(CamZ < MinZ) ||
			(MaxZ < CamZ))
		{
			return false;
		}

		return true;
	};
}