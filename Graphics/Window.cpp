#include "stdafx.h"
#include "Window.h"

Window* Window::Instance = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Window::handleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
	if (msg == WM_NCCREATE)
	{
		// extract ptr to window class from creation data
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWindow = static_cast<Window*>(pCreate->lpCreateParams);
		// set WinAPI-managed user data to store ptr to window instance
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
		// set message proc to normal (non-setup) handler now that setup is finished
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::handleMsgThunk));
		// forward message to window instance handler
		return pWindow->handleMsg(hWnd, msg, wParam, lParam);
	}
	// if we get a message before the WM_NCCREATE message, handle with default handler
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK Window::handleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// retrieve ptr to window instance
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	// forward message to window instance handler
	return pWnd->handleMsg(hWnd, msg, wParam, lParam);
}

LRESULT Window::handleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
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
			myKeyboard.clearState();
			break;
		}
	    case WM_ACTIVATE:
		{
			if (!cursorEnabled)
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
			if (!(lParam & 0x40000000) || myKeyboard.AutorepeatIsEnabled())
			{
				myKeyboard.onKeyPressed(static_cast<unsigned char>(wParam));
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
			myKeyboard.onKeyReleased(static_cast<unsigned char>(wParam));
			break;
		}
	    case WM_CHAR:
		{
		
			if (imio.WantCaptureKeyboard)
			{
				break;
			}
			myKeyboard.onChar(static_cast<unsigned char>(wParam));
			break;
		}
	    case WM_MOUSEMOVE:
		{
			const POINTS pt = MAKEPOINTS(lParam);
		
			if (!cursorEnabled)
			{
				if (!myMouse.IsInWindow())
				{
					SetCapture(hWnd);
					myMouse.OnMouseEnter();
					hideCursor();
				}
				break;
			}
		
			if (imio.WantCaptureMouse)
			{
				break;
			}
		
			if (pt.x >= 0 && pt.x < width && pt.y >= 0 && pt.y < height)
			{
				myMouse.OnMouseMove(pt.x, pt.y);
				if (!myMouse.IsInWindow())
				{
					SetCapture(hWnd);
					myMouse.OnMouseEnter();
				}
			}
			else
			{
				if (wParam & (MK_LBUTTON | MK_RBUTTON))
				{
					myMouse.OnMouseMove(pt.x, pt.y);
				}
				else
				{
					ReleaseCapture();
					myMouse.OnMouseLeave();
				}
			}
			break;
		}
	    case WM_LBUTTONDOWN:
		{
			SetForegroundWindow(hWnd);
			if (!cursorEnabled)
			{
				confineCursor();
				hideCursor();
			}
		
			if (imio.WantCaptureMouse)
			{
				break;
			}
			const POINTS pt = MAKEPOINTS(lParam);
			myMouse.OnLeftPressed(pt.x, pt.y);
			break;
		}
	    case WM_RBUTTONDOWN:
		{
		
			if (imio.WantCaptureMouse)
			{
				break;
			}
			const POINTS pt = MAKEPOINTS(lParam);
			myMouse.OnRightPressed(pt.x, pt.y);
			break;
		}
	    case WM_LBUTTONUP:
		{
		
			if (imio.WantCaptureMouse)
			{
				break;
			}
			const POINTS pt = MAKEPOINTS(lParam);
			myMouse.OnLeftReleased(pt.x, pt.y);
		
			if (pt.x < 0 || pt.x >= width || pt.y < 0 || pt.y >= height)
			{
				ReleaseCapture();
				myMouse.OnMouseLeave();
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
			myMouse.OnRightReleased(pt.x, pt.y);
		
			if (pt.x < 0 || pt.x >= width || pt.y < 0 || pt.y >= height)
			{
				ReleaseCapture();
				myMouse.OnMouseLeave();
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
			myMouse.OnWheelDelta(pt.x, pt.y, delta);
			break;
		}

	    case WM_INPUT:
	    {
		    if (!myMouse.RawEnabled())
		    {
			    break;
		    }
		    UINT size;
			if (GetRawInputData(
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
				myMouse.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
			}
			break;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);

}

Window::Window()
	: hInstance(GetModuleHandle(nullptr))
{
	Instance = this;
}

Window::~Window()
{
	ImGui_ImplWin32_Shutdown();
	DestroyWindow(windowHandle);
}

void Window::Initialize
(
	HINSTANCE hInstance, 
	int width, 
	int height, 
	const wchar_t* windowName, 
	const wchar_t* windowTitle, 
	BOOL fullscreen, 
	int ShowWnd
	) noexcept
{
	HRESULT hardwareResult = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	
	ASSERT_HR(hardwareResult);

	this->windowName = windowName;
	this->windowTitle = windowTitle;
	this->fullscreen = fullscreen;
	//if (fullscreen)
	//{
	//	HMONITOR hmon = MonitorFromWindow(windowHandle,
	//		MONITOR_DEFAULTTONEAREST);
	//	MONITORINFO mi = { sizeof(mi) };
	//	GetMonitorInfo(hmon, &mi);
	//	width = mi.rcMonitor.right - mi.rcMonitor.left;
	//	height = mi.rcMonitor.bottom - mi.rcMonitor.top;
	//}

	this->width = width;
	this->height = height;

	WNDCLASSEX WC;
	ZeroMemory(&WC, sizeof(WNDCLASSEX));
	WC.cbSize = sizeof(WNDCLASSEX);
	WC.style = CS_HREDRAW | CS_VREDRAW;
	WC.lpfnWndProc = handleMsgSetup;
	WC.hInstance = hInstance;
	WC.hIcon = static_cast<HICON>
		(
			LoadImage
			(
				hInstance, 
				MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 
				32, 32, 0
				)
			);
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

	windowHandle = CreateWindow
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
		hInstance,	// The app's handle
		0
		);

	ASSERT(windowHandle == nullptr);

	if (fullscreen)
	{
		SetWindowLong(windowHandle, GWL_STYLE, 0);
	}

	ShowWindow(windowHandle, ShowWnd);
	UpdateWindow(windowHandle);
	ImGui_ImplWin32_Init(windowHandle);

	RAWINPUTDEVICE RawInputDevice;
	RawInputDevice.usUsagePage = 0x01; // mouse page
	RawInputDevice.usUsage     = 0x02; // mouse usage
	RawInputDevice.dwFlags     = 0;
	RawInputDevice.hwndTarget  = nullptr;
	ASSERT(RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RawInputDevice)) == FALSE);
}

HWND Window::GetWindowHandle() noexcept
{
	return windowHandle;
}

void Window::StartMessagePump(std::function<void()> callback)
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
}

HINSTANCE Window::GetInstance() noexcept
{
	return Instance->GetInstance();
}

bool Window::IsFullscreen() noexcept
{
	return fullscreen;
}

SIZE Window::GetWindowSize() noexcept
{
	return SIZE{ width, height };
}

void Window::RegisterOnResizeCallback(std::function<void()> callback) noexcept
{
	onResizeCallbacks.push_back(callback);
}

void Window::OnResize() noexcept
{
	for (auto& resizeCallback : onResizeCallbacks)
	{
		resizeCallback();
	}
}


///////////////////////////////////////////
// Device Stuff

void Window::EnableCursor() noexcept
{
	cursorEnabled = true;
	showCursor();
	enableImGuiMouse();
	freeCursor();
}

void Window::DisableCursor() noexcept
{
	cursorEnabled = false;
	hideCursor();
	disableImGuiMouse();
	confineCursor();
}

bool Window::GetCursorEnabled() const noexcept
{
	return cursorEnabled;
}

void Window::confineCursor() noexcept
{
	RECT rect;
	GetClientRect(GetWindowHandle(), &rect);
	MapWindowPoints(GetWindowHandle(), nullptr, reinterpret_cast<POINT*>(&rect), 2);
	ClipCursor(&rect);
}

void Window::freeCursor() noexcept
{
	ClipCursor(nullptr);
}

void Window::showCursor() noexcept
{
	while (::ShowCursor(TRUE) < 0);
}

void Window::hideCursor() noexcept
{
	while (0 <= ::ShowCursor(FALSE));
}

void Window::enableImGuiMouse() noexcept
{
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
}

void Window::disableImGuiMouse() noexcept
{
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
}