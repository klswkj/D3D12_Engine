#include "stdafx.h"
#include "ImGuiManager.h"
#include "imgui.h"
#include "Window.h"
#include "Device.h"
#include "BufferManager.h"

namespace imguiManager
{
	bool bImguiEnabled = false;

	void Initialize()
	{
		if (!std::filesystem::exists("imgui.ini") && std::filesystem::exists("imgui_default.ini"))
		{
			std::filesystem::copy_file("imgui_default.ini", "imgui.ini");
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplDX12_Init
		(
			device::g_pDevice, device::g_DisplayBufferCount, bufferManager::g_SceneColorBuffer.GetFormat(),
			device::g_ImguiFontHeap->GetCPUDescriptorHandleForHeapStart(),
			device::g_ImguiFontHeap->GetGPUDescriptorHandleForHeapStart()
		);

		ImGui_ImplWin32_Init(window::g_hWnd);
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

		bImguiEnabled = true;
	}

	void Destroy()
	{
		if (bImguiEnabled)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui::DestroyContext();
			ImGui_ImplWin32_Shutdown();
		}
	}
}