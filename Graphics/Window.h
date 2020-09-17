#pragma once

class MyKeyboard;
class MyMouse;

namespace window
{
	extern uint32_t g_windowWidth;
	extern uint32_t g_windowHeight;

	void Initialize
	(
		const wchar_t* windowName = L"KSK", 
		const wchar_t* windowTitle = L"D3D12_Engine", 
		uint32_t width = g_windowWidth, 
		uint32_t height = g_windowHeight, 
		BOOL g_fullscreen = false, 
		int ShowWnd = 1
	);

	HWND				GetWindowHandle() noexcept;
	void				StartMessagePump(std::function<void()> callback);

	void EnableCursor() noexcept; 
	void DisableCursor() noexcept;

	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void confineCursor() noexcept;
	void freeCursor() noexcept;
	void showCursor() noexcept;
	void hideCursor() noexcept;
	void enableImGuiMouse() noexcept;
	void disableImGuiMouse() noexcept;
	// extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	extern HWND g_hWnd;
	extern MyKeyboard        g_Keyboard;
	extern MyMouse           g_Mouse;

	constexpr static BOOL		g_fullscreen{ false };
	static BOOL                 g_bCursorEnabled{ true };

	// rawinput data scpoed
	// std::vector<std::function<void()>> onResizeCallbacks = {}; // Swapchain, Resize
};