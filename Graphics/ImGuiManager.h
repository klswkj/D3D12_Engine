#pragma once
#include "imgui.h"
#include <filesystem>

namespace imguiManager
{
	void Initialize()
	{
		namespace fs = std::filesystem;
		if (!fs::exists("imgui.ini") && fs::exists("imgui_default.ini"))
		{
			fs::copy_file("imgui_default.ini", "imgui.ini");
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
	}

	void Destroy()
	{
		ImGui::DestroyContext();
	}
};
