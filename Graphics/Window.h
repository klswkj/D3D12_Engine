#pragma once

class MyKeyboard;
class MyMouse;

namespace window
{
	constexpr uint32_t g_TargetWindowWidth = 1920u;
	constexpr uint32_t g_TargetWindowHeight = 1080u;

	extern HWND         g_hWnd;
	extern MyMouse      g_Mouse;
	extern MyKeyboard   g_Keyboard;

	void Initialize
	(
		const wchar_t* windowName = L"KSK", 
		const wchar_t* windowTitle = L"D3D12_Engine", 
		uint32_t width = g_TargetWindowWidth, 
		uint32_t height = g_TargetWindowHeight, 
		BOOL g_fullscreen = false, 
		int ShowWnd = 1
	);
	void Destroy();

	HWND GetWindowHandle() noexcept;
	void StartMessagePump(std::function<void()> callback);

	void EnableCursor() noexcept; 
	void DisableCursor() noexcept;

	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	bool bCursorEnabled() noexcept;
	void confineCursor() noexcept;
	void freeCursor() noexcept;
	void showCursor() noexcept;
	void hideCursor() noexcept;
	void enableImGuiMouse() noexcept;
	void disableImGuiMouse() noexcept;
	// extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


	constexpr static BOOL		g_fullscreen = false;
	static BOOL                 g_bCursorEnabled = true;

	// rawinput data scpoed
	// std::vector<std::function<void()>> onResizeCallbacks = {}; // Swapchain, Resize
};