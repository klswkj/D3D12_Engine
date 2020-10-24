#include "stdafx.h"
#include "Window.h"
#include "Graphics.h"
#include "WindowInput.h"
#include "MyMouse.h"
#include "MyKeyboard.h"
#include "imgui_impl_win32.h"

// https://billthefarmer.github.io/blog/post/handling-resizing-in-windows/#:~:text=The%20WM_SIZE%20message%20is%20sent,in%20the%20WindowProc%20callback%20function
// TODO : Window Sizing with handler

namespace window
{
	HWND g_hWnd = nullptr;

	std::vector<byte> s_rawInputData;
	MyKeyboard        g_Keyboard;
	MyMouse           g_Mouse;

	// LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		// retrieve ptr to window instance
		LONG_PTR pWnd = GetWindowLongPtr(hWnd, GWLP_USERDATA);
		// forward message to window instance handler
		return HandleMsg(hWnd, msg, wParam, lParam);
	}

	LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
		if (msg == WM_NCCREATE)
		{
			// extract ptr to window class from creation data
			const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
			LPVOID temp = pCreate->lpCreateParams;
			// set WinAPI-managed user data to store ptr to window instance
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(temp));
			// set message proc to normal (non-setup) handler now that setup is finished
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&HandleMsgThunk));
			// forward message to window instance handler
			return HandleMsg(hWnd, msg, wParam, lParam);
		}
		// if we get a message before the WM_NCCREATE message, handle with default handler
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		{
			return true;
		}

		const auto& imio = ImGui::GetIO();

		switch (msg)
		{
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_KILLFOCUS:
		{
			g_Keyboard.ClearState();
			windowInput::Update(graphics::GetFrameTime());
			break;
		}
		case WM_ACTIVATE:
		{
			if (!g_bCursorEnabled)
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
			if (imio.WantCaptureKeyboard)
			{
				break;
			}
			if (!(lParam & 0x40000000) || g_Keyboard.AutorepeatIsEnabled())
			{
				g_Keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
			}
			break;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			if (imio.WantCaptureKeyboard)
			{
				break;
			}
			g_Keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
			break;
		}
		case WM_CHAR:
		{
			if (imio.WantCaptureKeyboard)
			{
				break;
			}
			g_Keyboard.OnChar(static_cast<unsigned char>(wParam));
			break;
		}
		case WM_MOUSEMOVE:
		{
			const POINTS pt = MAKEPOINTS(lParam);

			if (!g_bCursorEnabled)
			{
				if (!g_Mouse.IsInWindow())
				{
					SetCapture(hWnd);
					g_Mouse.OnMouseEnter();
					hideCursor();
				}
				break;
			}

			if (imio.WantCaptureMouse)
			{
				break;
			}

			if (0 <= pt.x && pt.x < g_TargetWindowWidth && 0 <= pt.y && pt.y < g_TargetWindowHeight)
			{
				g_Mouse.OnMouseMove(pt.x, pt.y);
				if (!g_Mouse.IsInWindow())
				{
					SetCapture(hWnd);
					g_Mouse.OnMouseEnter();
				}
			}
			else
			{
				if (wParam & (MK_LBUTTON | MK_RBUTTON))
				{
					g_Mouse.OnMouseMove(pt.x, pt.y);
				}
				else
				{
					ReleaseCapture();
					g_Mouse.OnMouseLeave();
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			SetForegroundWindow(hWnd);
			if (!g_bCursorEnabled)
			{
				confineCursor();
				hideCursor();
			}

			if (imio.WantCaptureMouse)
			{
				break;
			}
			const POINTS pt = MAKEPOINTS(lParam);
			g_Mouse.OnLeftPressed(pt.x, pt.y);
			break;
		}
		case WM_RBUTTONDOWN:
		{
			if (imio.WantCaptureMouse)
			{
				break;
			}
			const POINTS pt = MAKEPOINTS(lParam);
			g_Mouse.OnRightPressed(pt.x, pt.y);
			break;
		}
		case WM_LBUTTONUP:
		{
			if (imio.WantCaptureMouse)
			{
				break;
			}
			const POINTS pt = MAKEPOINTS(lParam);
			g_Mouse.OnLeftReleased(pt.x, pt.y);

			if (pt.x < 0 || g_TargetWindowWidth <= pt.x || pt.y < 0 || g_TargetWindowHeight <= pt.y)
			{
				ReleaseCapture();
				g_Mouse.OnMouseLeave();
			}
			break;
		}
		case WM_RBUTTONUP:
		{
			if (imio.WantCaptureMouse)
			{
				break;
			}
			const POINTS pt = MAKEPOINTS(lParam);
			g_Mouse.OnRightReleased(pt.x, pt.y);

			if (pt.x < 0 || g_TargetWindowWidth <= pt.x || pt.y < 0 || g_TargetWindowHeight <= pt.y)
			{
				ReleaseCapture();
				g_Mouse.OnMouseLeave();
			}
			break;
		}
		case WM_MOUSEWHEEL:
		{
			// stifle this mouse message if imgui wants to capture
			if (imio.WantCaptureMouse)
			{
				break;
			}
			const POINTS pt = MAKEPOINTS(lParam);
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			g_Mouse.OnWheelDelta(pt.x, pt.y, delta);
			break;
		}

		case WM_INPUT:
		{
			if (!g_Mouse.RawEnabled())
			{
				break;
			}
			UINT size;
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
				nullptr, &size, sizeof(RAWINPUTHEADER)) == -1)
			{
				break;
			}

			s_rawInputData.resize(size);

			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
				s_rawInputData.data(), &size, sizeof(RAWINPUTHEADER)) != size)
			{
				break;
			}

			auto& ri = reinterpret_cast<const RAWINPUT&>(*s_rawInputData.data());

			if (ri.header.dwType == RIM_TYPEMOUSE &&
				(ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
			{
				g_Mouse.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
			}
			break;
		}

		case WM_SIZE:
		{
			// graphics::Resize((UINT)(UINT64)lParam & 0xFFFF, (UINT)(UINT64)lParam >> 16);
			device::Resize((UINT)(UINT64)lParam & 0xFFFF, (UINT)(UINT64)lParam >> 16);
			break;
		}
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		return 0;
	}

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
		HRESULT hardwareResult = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

		ASSERT_HR(hardwareResult);

		WNDCLASSEX WC;
		ZeroMemory(&WC, sizeof(WNDCLASSEX));
		WC.cbSize = sizeof(WNDCLASSEX);
		WC.style = CS_HREDRAW | CS_VREDRAW;
		WC.lpfnWndProc = HandleMsgSetup;
		WC.hInstance = GetModuleHandle(0);
		WC.hIcon = static_cast<HICON>(LoadImage(
					WC.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
					32, 32, 0));
		WC.hIconSm = WC.hIcon;
		WC.hCursor = LoadCursor(NULL, IDC_ARROW);
		WC.hbrBackground = (HBRUSH)COLOR_WINDOW;
		WC.lpszClassName = windowName;

		ASSERT(0 != RegisterClassEx(&WC), "Unable to register a window");

		RECT clientRect;
		SetRect(&clientRect, 0, 0, width, height);
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

		g_hWnd = CreateWindow // WM_NCCREATE -> WM_NCCALCSIZE -> WM_CREATE -> WM_SHOWWINDOW -> WM_WINDOWPOSCHANGING -> WM_ACTIVATEAPP -> WM_NCACTIVATE ->
			                 // WM_GETICON -> WM_ACTIVATE -> WM_IME_SETCONTEXT -> WM_IME_NOTIFY -> WM_SETFOCUS -> WM_NCPAINT -> WM_ERASEBKGND
			                 // -> WM_WINDOWPOSCHANGED -> WM_SIZE -> WM_MOVE
		(
			WC.lpszClassName,
			windowTitle,
			WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top,
			nullptr,
			nullptr,
			WC.hInstance,
			0
		);

		ASSERT(g_hWnd != nullptr);

		if (g_fullscreen)
		{
			SetWindowLong(g_hWnd, GWL_STYLE, 0);
		}

		ImGui_ImplWin32_Init(g_hWnd);

		RAWINPUTDEVICE RawInputDevice;
		RawInputDevice.usUsagePage = 0x01; // mouse page
		RawInputDevice.usUsage = 0x02; // mouse usage
		RawInputDevice.dwFlags = 0;
		RawInputDevice.hwndTarget = nullptr;
		ASSERT(RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RawInputDevice)) != FALSE);
	}

	void Destroy()
	{
		ImGui_ImplWin32_Shutdown();
		DestroyWindow(g_hWnd);
		CoUninitialize();
	}

	HWND GetWindowHandle() noexcept
	{
		return g_hWnd;
	}

	void StartMessagePump(std::function<void()> callback)
	{
		MSG msg = {};

		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				callback();
			}
		}
	}

	///////////////////////////////////////////
	// Device Stuff

	void EnableCursor() noexcept
	{
		g_bCursorEnabled = true;
		showCursor();
		enableImGuiMouse();
		freeCursor();
	}
	bool bCursorEnabled() noexcept
	{
		return g_bCursorEnabled;
	}
	void DisableCursor() noexcept
	{
		g_bCursorEnabled = false;
		hideCursor();
		disableImGuiMouse();
		confineCursor();
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
}