#pragma once
#include "MyKeyboard.h"
#include "MyMouse.h"

namespace window
{
	void				Initialize
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

	extern HWND g_hWnd;

	std::vector<byte> rawInputData;

	MyKeyboard        g_Keyboard;
	MyMouse           g_Mouse;

	constexpr static uint32_t          g_windowWidth{ 720 };
	constexpr static uint32_t          g_windowHeight{ 480 };
	constexpr static BOOL		       g_fullscreen{ false };
	static BOOL                        g_bCursorEnabled{ true };

	// rawinput data scpoed
	// std::vector<std::function<void()>> onResizeCallbacks = {}; // Swapchain, Resize
};