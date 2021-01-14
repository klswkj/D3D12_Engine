#pragma once

#include "MyMouse.h"
#include "MyKeyboard.h"

namespace window
{
	class CWindow;

	constexpr uint32_t g_TargetWindowWidth = 1280u;
	constexpr uint32_t g_TargetWindowHeight = 720u;

	extern HWND         g_hWnd;
	extern CWindow*     g_pCWindow;

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
	// void StartMessagePump(std::function<void()> callback);

	void EnableCursor() noexcept; 
	void DisableCursor() noexcept;

	// LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	bool bCursorEnabled() noexcept;
	void confineCursor() noexcept;
	void freeCursor() noexcept;
	void showCursor() noexcept;
	void hideCursor() noexcept;
	void enableImGuiMouse() noexcept;
	void disableImGuiMouse() noexcept;
	// extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// std::vector<std::function<void()>> onResizeCallbacks = {}; // Swapchain, Resize

	class CWindow
	{
	public:
		// static method

	public:
		CWindow
		(
			const wchar_t* WindowName, // = L"KSK",
			const wchar_t* WindowTitle, // = L"D3D12_Engine",
			uint32_t width = g_TargetWindowWidth,
			uint32_t height = g_TargetWindowHeight,
			BOOL g_fullscreen = true,
			int ShowWnd = 1
		);
		~CWindow();
		CWindow(const CWindow&) = delete;
		CWindow& operator=(const CWindow&) = delete;

		void SetTitle(const std::wstring& _Title);

		void StartMessagePump(std::function<void()> callback);

		static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
		static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
		LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
		// LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HWND GetWindowHandle() noexcept;

		void EnableCursor() noexcept;
		void DisableCursor() noexcept;
		bool GetCursorEnabled() const noexcept;

		void ConfineCursor() noexcept;
		void FreeCursor() noexcept;
		void ShowCursor() noexcept;
		void HideCursor() noexcept;
		void EnableImGuiMouse() noexcept;
		void DisableImGuiMouse() noexcept;

	public:
		HWND       m_HWND;
		MyKeyboard m_Keyboard;
		MyMouse    m_Mouse;
		
		UINT       m_WindowWidth;
		UINT       m_WindowHeight;
		bool       m_bCursorEnabled;
		bool       m_bFullScreen;

	private:

	private:
		std::vector<byte> m_RawInputData;
	};
};