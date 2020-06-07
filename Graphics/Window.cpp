#include "stdafx.h"
#include "Window.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace window
{
	uint32_t g_windowWidth{ 720 };
	uint32_t g_windowHeight{ 480 };

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
		{
			break;
		}
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
		{
			// break;
		}
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

			if (0 <= pt.x && pt.x < g_windowWidth && 0 <= pt.y && pt.y < g_windowHeight)
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

			if (pt.x < 0 || g_windowWidth <= pt.x || pt.y < 0 || g_windowHeight <= pt.y)
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

			if (pt.x < 0 || g_windowWidth <= pt.x || pt.y < 0 || g_windowHeight <= pt.y)
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
			if(GetRawInputData
			(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
				nullptr,
				&size,
				sizeof(RAWINPUTHEADER)
			) == -1)
			{
				break;
			}

			rawInputData.resize(size);

			if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
				rawInputData.data(),
				&size,
				sizeof(RAWINPUTHEADER)) != size)
			{
				break;
			}

			auto& ri = reinterpret_cast<const RAWINPUT&>(*rawInputData.data());
			if (ri.header.dwType == RIM_TYPEMOUSE &&
				(ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
			{
				g_Mouse.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
			}
			break;
		}
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);

	}

	void Initialize
	(
		const wchar_t* windowName,  // = L"KSK",
		const wchar_t* windowTitle, // = L"D3D12_Engine",
		uint32_t width,             // = g_windowWidth,
		uint32_t height,            // = g_windowHeight,
		BOOL g_fullscreen,          // = false,
		int ShowWnd                 // = 1
	) noexcept
	{
		HWND g_hWnd = nullptr;

		HRESULT hardwareResult = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);

		ASSERT_HR(hardwareResult);

		WNDCLASSEX WC;
		ZeroMemory(&WC, sizeof(WNDCLASSEX));
		WC.cbSize = sizeof(WNDCLASSEX);
		WC.style = CS_HREDRAW | CS_VREDRAW;
		WC.lpfnWndProc = HandleMsgSetup;
		WC.hInstance = GetModuleHandle(0);
		WC.hIcon = static_cast<HICON>(LoadImage
		(
			WC.hInstance,
			MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
			32, 32, 0
		));
		WC.hIconSm = WC.hIcon;
		WC.hCursor = LoadCursor(NULL, IDC_ARROW);
		WC.hbrBackground = (HBRUSH)COLOR_WINDOW;
		WC.lpszClassName = windowName;

		RegisterClassEx(&WC);

		///////////////////////////////

		RECT clientRect;
		SetRect(&clientRect, 0, 0, width, height);
		AdjustWindowRect
		(
			&clientRect,
			WS_OVERLAPPEDWINDOW,	// Has a title bar, border, min and max buttons, etc.
			false
		);

		RECT desktopRect;
		GetClientRect(GetDesktopWindow(), &desktopRect);
		int centeredX = (desktopRect.right / 2) - (clientRect.right / 2);
		int centeredY = (desktopRect.bottom / 2) - (clientRect.bottom / 2);

		g_hWnd = CreateWindow
		(
			WC.lpszClassName,
			windowTitle,
			WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
			CW_USEDEFAULT, // centeredX,
			CW_USEDEFAULT, // centeredY,
			clientRect.right - clientRect.left,	// Calculated width
			clientRect.bottom - clientRect.top,	// Calculated height
			nullptr,			// No parent window
			nullptr,			// No menu
			WC.hInstance,	// The app's handle
			0
		);

		ASSERT(g_hWnd == nullptr);

		if (g_fullscreen)
		{
			SetWindowLong(g_hWnd, GWL_STYLE, 0);
		}

		ShowWindow(g_hWnd, ShowWnd);
		ImGui_ImplWin32_Init(g_hWnd);

		RAWINPUTDEVICE RawInputDevice;
		RawInputDevice.usUsagePage = 0x01; // mouse page
		RawInputDevice.usUsage = 0x02; // mouse usage
		RawInputDevice.dwFlags = 0;
		RawInputDevice.hwndTarget = nullptr;
		ASSERT(RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RawInputDevice)) == FALSE);
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
			//else
			{
				callback();
			}
		}

		ImGui_ImplWin32_Shutdown();
		DestroyWindow(g_hWnd);
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