#include "stdafx.h"
#include "Window.h"
#include "Graphics.h"
#include "WindowInput.h"
#include "imgui_impl_win32.h"
#include "ImGuiManager.h"

// https://billthefarmer.github.io/blog/post/handling-resizing-in-windows/#:~:text=The%20WM_SIZE%20message%20is%20sent,in%20the%20WindowProc%20callback%20function
// TODO : Window Sizing with handler

namespace window
{
	HWND     g_hWnd     = nullptr;
	CWindow* g_pCWindow = nullptr;

	void Initialize
	(
		const wchar_t* windowName,  // = L"KSK",
		const wchar_t* windowTitle, // = L"D3D12_Engine",
		uint32_t width,             // = g_TargetWindowWidth,
		uint32_t height,            // = g_TargetWindowHeight,
		BOOL g_fullscreen,          // = false,
		int ShowWnd                 // = 1
	)
	{
		ASSERT(g_pCWindow == nullptr);
		g_pCWindow = new CWindow(windowName, windowTitle, width, height, g_fullscreen, ShowWnd);
	}
	void Destroy()
	{
		::DestroyWindow(g_hWnd);
		// ::UnregisterClass()
		::CoUninitialize();

		if (g_pCWindow)
		{
			delete g_pCWindow;
		}
	}
	HWND GetWindowHandle() noexcept
	{
		return g_hWnd;
	}

	///////////////////////////////////////////
	// Device Stuff
	void EnableCursor() noexcept
	{
		g_pCWindow->EnableCursor();
	}
	bool bCursorEnabled() noexcept
	{
		return g_pCWindow->m_bCursorEnabled;
	}
	void DisableCursor() noexcept
	{
		g_pCWindow->DisableCursor();
	}
	void confineCursor() noexcept
	{
		RECT rect;
		GetClientRect(GetWindowHandle(), &rect);
		MapWindowPoints(GetWindowHandle(), nullptr, reinterpret_cast<POINT*>(&rect), 2);
		ClipCursor(&rect);
	}
	void freeCursor() noexcept
	{
		ClipCursor(nullptr);
	}
	void showCursor() noexcept
	{
		while (::ShowCursor(TRUE) < 0);
	}
	void hideCursor() noexcept
	{
		while (0 <= ::ShowCursor(FALSE));
	}
	void enableImGuiMouse() noexcept
	{
		ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}
	void disableImGuiMouse() noexcept
	{
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}

	CWindow::CWindow
	(
		const wchar_t* WindowName, // = L"KSK",
		const wchar_t* WindowTitle, // = L"D3D12_Engine",
		uint32_t Width,
		uint32_t Height,
		BOOL Fullscreen, //  = false,
		int ShowWnd // = 1
	)
	{
		WNDCLASSEX WC;
		ZeroMemory(&WC, sizeof(WNDCLASSEX));
		WC.cbSize        = sizeof(WNDCLASSEX);
		WC.style         = CS_CLASSDC; // CS_HREDRAW | CS_VREDRAW;
		WC.lpfnWndProc   = CWindow::HandleMsgSetup;
		WC.cbClsExtra    = 0l;
		WC.cbWndExtra    = 0l;
		WC.hInstance     = GetModuleHandle(NULL);
		WC.hIcon         = static_cast<HICON>(LoadImage(WC.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0));
		WC.hCursor       = LoadCursor(NULL, IDC_ARROW);
		WC.hbrBackground = (HBRUSH)COLOR_WINDOW;
		WC.lpszMenuName  = WindowTitle;
		WC.lpszClassName = WindowName;
		WC.hIconSm       = WC.hIcon;

		ASSERT(::RegisterClassEx(&WC) != 0, "Unable to register a window");
		ASSERT_HR(::CoInitializeEx(nullptr, COINITBASE_MULTITHREADED));

		/*
		RECT clientRect;
		SetRect(&clientRect, 0, 0, Width, Height);
		AdjustWindowRect
		(
			&clientRect,
			// WS_OVERLAPPEDWINDOW,
			WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
			false
		);
		
		RECT desktopRect;
		GetClientRect(GetDesktopWindow(), &desktopRect);

		int centeredX = (clientRect.right - desktopRect.right) / 2;
		int centeredY = (clientRect.bottom - desktopRect.bottom) / 2;
		*/

		g_hWnd = ::CreateWindow
		(
			WindowName,
			WindowTitle,
			WS_OVERLAPPEDWINDOW, // WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
			100,    // CW_USEDEFAULT,
			100,    // CW_USEDEFAULT,
			Width,  // clientRect.right - clientRect.left,
			Height, // clientRect.bottom - clientRect.top,
			nullptr,
			nullptr,
			WC.hInstance,
			this
		);

		ASSERT(g_hWnd != nullptr);

		if (Fullscreen)
		{
			::SetWindowLong(g_hWnd, GWL_STYLE, 0);
		}

		RAWINPUTDEVICE RawInputDevice;
		RawInputDevice.usUsagePage = 0x01; // mouse page
		RawInputDevice.usUsage = 0x02; // mouse usage
		RawInputDevice.dwFlags = 0;
		RawInputDevice.hwndTarget = nullptr;
		ASSERT(RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RawInputDevice)) != FALSE);
		
		if (!RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RawInputDevice)))
		{
			DWORD LastError = ::GetLastError();
			ASSERT(!LastError);
		}


		m_bFullScreen    = Fullscreen;
		m_bCursorEnabled = true;
		m_HWND           = g_hWnd;
		m_WindowWidth    = Width;
		m_WindowHeight   = Height;
	}
	CWindow::~CWindow()
	{
	}

	void CWindow::SetTitle(const std::wstring& _Title)
	{
		ASSERT(::SetWindowText(g_hWnd, _Title.c_str()));
	}

	void CWindow::StartMessagePump(std::function<void()> callback)
	{
		MSG msg = {};

		while (msg.message != WM_QUIT)
		{
			if (::PeekMessage(&msg, NULL, 0u, 0u, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				continue;
			}
			else
			{
				callback();
			}
		}
	}

	// LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK CWindow::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		// retrieve ptr to window instance
		LONG_PTR pWnd = ::GetWindowLongPtr(hWnd, GWLP_USERDATA);

		// forward message to window instance handler
		return g_pCWindow->HandleMsg(hWnd, msg, wParam, lParam);
	}
	LRESULT CALLBACK CWindow::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
		if (msg == WM_NCCREATE)
		{
			// extract ptr to window class from creation data
			const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
			LPVOID temp = pCreate->lpCreateParams;

			// set WinAPI-managed user data to store ptr to window instance
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(temp));

			// set message proc to normal (non-setup) handler now that setup is finished
			::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&CWindow::HandleMsgThunk));

			// forward message to window instance handler
			return g_pCWindow->HandleMsg(hWnd, msg, wParam, lParam);
		}

		// if we get a message before the WM_NCCREATE message, handle with default handler
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}

	HWND CWindow::GetWindowHandle() noexcept
	{
		return g_hWnd;
	}

	void CWindow::EnableCursor() noexcept
	{
		m_bCursorEnabled = true;
		showCursor();
		enableImGuiMouse();
		freeCursor();
	}
	void CWindow::DisableCursor() noexcept
	{
		m_bCursorEnabled = false;
		hideCursor();
		disableImGuiMouse();
		confineCursor();
	}
	bool CWindow::GetCursorEnabled() const noexcept
	{
		return m_bCursorEnabled;
	}

	void CWindow::ConfineCursor() noexcept
	{
		RECT rect;
		GetClientRect(GetWindowHandle(), &rect);
		MapWindowPoints(GetWindowHandle(), nullptr, reinterpret_cast<POINT*>(&rect), 2);
		ClipCursor(&rect);
	}
	void CWindow::FreeCursor() noexcept
	{
		ClipCursor(nullptr);
	}
	void CWindow::ShowCursor() noexcept
	{
		while (::ShowCursor(TRUE) < 0);
	}
	void CWindow::HideCursor() noexcept
	{
		while (0 <= ::ShowCursor(FALSE));
	}
	void CWindow::EnableImGuiMouse() noexcept
	{
		ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}
	void CWindow::DisableImGuiMouse() noexcept
	{
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}

	LRESULT CWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		{
			return true;
		}

		// const auto& imio = ImGui::GetIO();

		switch (msg)
		{
		case WM_CLOSE:
		{
			::PostQuitMessage(0);
			return 0;
		}
		case WM_KILLFOCUS:
		{
			m_Keyboard.ClearState();
			windowInput::Update(graphics::GetFrameTime());
			break;
		}
		case WM_ACTIVATE:
		{
			if (!m_bCursorEnabled)
			{
				if (wParam & WA_ACTIVE)
				{
					confineCursor();
					hideCursor();
				}
				else
				{
					freeCursor();
					showCursor();
				}
			}
			break;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			if (imguiManager::bImguiEnabled)
			{
				const auto& imio = ImGui::GetIO();

				if (imio.WantCaptureKeyboard)
				{
					break;
				}
			}
			if (!(lParam & 0x40000000) || m_Keyboard.AutorepeatIsEnabled())
			{
				m_Keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
			}
			break;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			if (imguiManager::bImguiEnabled)
			{
				const auto& imio = ImGui::GetIO();

				if (imio.WantCaptureKeyboard)
				{
					break;
				}
			}
			m_Keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
			break;
		}
		case WM_CHAR:
		{
			if (imguiManager::bImguiEnabled)
			{
				const auto& imio = ImGui::GetIO();

				if (imio.WantCaptureKeyboard)
				{
					break;
				}
			}
			m_Keyboard.OnChar(static_cast<unsigned char>(wParam));
			break;
		}
		case WM_MOUSEMOVE:
		{
			const POINTS pt = MAKEPOINTS(lParam);

			if (!m_bCursorEnabled)
			{
				if (!m_Mouse.IsInWindow())
				{
					SetCapture(hWnd);
					m_Mouse.OnMouseEnter();
					hideCursor();
				}
				break;
			}

			if (imguiManager::bImguiEnabled)
			{
				const auto& imio = ImGui::GetIO();

				if (imio.WantCaptureMouse)
				{
					break;
				}
			}

			if (0 <= pt.x && pt.x < device::g_DisplayWidth && 0 <= pt.y && pt.y < device::g_DisplayHeight)
			{
				m_Mouse.OnMouseMove(pt.x, pt.y);

				if (!m_Mouse.IsInWindow())
				{
					SetCapture(hWnd);
					m_Mouse.OnMouseEnter();
				}
			}
			else
			{
				if (wParam & (MK_LBUTTON | MK_RBUTTON))
				{
					m_Mouse.OnMouseMove(pt.x, pt.y);
				}
				else
				{
					ReleaseCapture();
					m_Mouse.OnMouseLeave();
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			::SetForegroundWindow(hWnd);
			if (!m_bCursorEnabled)
			{
				confineCursor();
				hideCursor();
			}

			if (imguiManager::bImguiEnabled)
			{
				const auto& ImGuiIO = ImGui::GetIO();

				if (ImGuiIO.WantCaptureMouse)
				{
					break;
				}
			}
			const POINTS pt = MAKEPOINTS(lParam);
			m_Mouse.OnLeftPressed(pt.x, pt.y);
			break;
		}
		case WM_RBUTTONDOWN:
		{
			if (imguiManager::bImguiEnabled)
			{
				const auto& imio = ImGui::GetIO();

				if (imio.WantCaptureMouse)
				{
					break;
				}
			}
			const POINTS pt = MAKEPOINTS(lParam);
			m_Mouse.OnRightPressed(pt.x, pt.y);
			break;
		}
		case WM_LBUTTONUP:
		{
			if (imguiManager::bImguiEnabled)
			{
				const auto& imio = ImGui::GetIO();

				if (imio.WantCaptureMouse)
				{
					break;
				}
			}
			const POINTS pt = MAKEPOINTS(lParam);
			m_Mouse.OnLeftReleased(pt.x, pt.y);

			// if (pt.x < 0 || g_TargetWindowWidth <= pt.x || pt.y < 0 || g_TargetWindowHeight <= pt.y)
			if (pt.x < 0 || device::g_DisplayWidth <= pt.x || pt.y < 0 || device::g_DisplayHeight <= pt.y)
			{
				::ReleaseCapture();
				m_Mouse.OnMouseLeave();
			}
			break;
		}
		case WM_RBUTTONUP:
		{
			if (imguiManager::bImguiEnabled)
			{
				const auto& imio = ImGui::GetIO();

				if (imio.WantCaptureMouse)
				{
					break;
				}
			}
			const POINTS pt = MAKEPOINTS(lParam);
			m_Mouse.OnRightReleased(pt.x, pt.y);

			if (pt.x < 0 || device::g_DisplayWidth <= pt.x || pt.y < 0 || device::g_DisplayHeight <= pt.y)
			{
				::ReleaseCapture();
				m_Mouse.OnMouseLeave();
			}
			break;
		}
		case WM_MOUSEWHEEL:
		{
			if (imguiManager::bImguiEnabled)
			{
				const auto& imio = ImGui::GetIO();

				if (imio.WantCaptureMouse)
				{
					break;
				}
			}
			const POINTS pt = MAKEPOINTS(lParam);
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			m_Mouse.OnWheelDelta(pt.x, pt.y, delta);
			break;
		}

		case WM_INPUT:
		{
			if (!m_Mouse.RawEnabled())
			{
				break;
			}
			UINT size;
			if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
				nullptr, &size, sizeof(RAWINPUTHEADER)) == -1)
			{
				break;
			}

			m_RawInputData.resize(size);

			if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
				m_RawInputData.data(), &size, sizeof(RAWINPUTHEADER)) != size)
			{
				break;
			}

			auto& ri = reinterpret_cast<const RAWINPUT&>(*m_RawInputData.data());

			if (ri.header.dwType == RIM_TYPEMOUSE &&
				(ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
			{
				m_Mouse.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
			}
			break;
		}

		case WM_SIZE:
		{
			if (wParam != SIZE_MINIMIZED)
			{
				ImGui_ImplDX12_InvalidateDeviceObjects();

				UINT width  = LOWORD(lParam);
				UINT height = HIWORD(lParam);
				device::Resize(width, height);

				ImGui_ImplDX12_CreateDeviceObjects();
			}
			
			break;
		}
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		return 0;
	}
}