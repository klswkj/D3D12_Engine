#include "Window.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "resource.h"
#include <windowsx.h>
#include <wrl.h>
#include <iostream>
#include <sstream>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

using namespace DirectX;

Window* Window::Instance = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowsProcessMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
	return Window::Instance->WindowsMessageCallback(hWnd, msg, wParam, lParam);
}

Window::~Window()
{
	ImGui_ImplWin32_Shutdown();
	DestroyWindow(windowHandle);
}

LRESULT Window::WindowsMessageCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		width = LOWORD(lParam);
		height = HIWORD(lParam);
		OnResize();
		return 0;
	case WM_ACTIVATEAPP:
		Keyboard::ProcessMessage(msg, wParam, lParam);
		Mouse::ProcessMessage(msg, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Keyboard::ProcessMessage(msg, wParam, lParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		//OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		Mouse::ProcessMessage(msg, wParam, lParam);
		return 0;

		// Mouse button being released (while the cursor is currently over our window)
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		//OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		Mouse::ProcessMessage(msg, wParam, lParam);
		return 0;

		// Cursor moves over the window (or outside, while we're currently capturing it)
	case WM_MOUSEMOVE:
		//OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		Mouse::ProcessMessage(msg, wParam, lParam);
		return 0;

		// Mouse wheel is scrolled
	case WM_MOUSEWHEEL:
		//OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		Mouse::ProcessMessage(msg, wParam, lParam);
		return 0;
	case WM_INPUT:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		Mouse::ProcessMessage(msg, wParam, lParam);
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Window::Initialize(HINSTANCE hInstance, int width, int height, const wchar_t* windowName, const wchar_t* windowTitle, BOOL fullscreen, int ShowWnd)
{
	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	if (FAILED(hr))
	{
	}
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
	WC.lpfnWndProc = WindowsProcessMessage;
	WC.hInstance = hInstance;
	WC.hIcon = static_cast<HICON>
		(
			LoadImage
			(
				hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0
				)
			);
	WC.hIconSm = WC.hIcon;
	WC.hCursor = LoadCursor(NULL, IDC_ARROW);
	WC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	WC.lpszClassName = windowName;

	RegisterClassEx(&WC);
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
		WS_OVERLAPPEDWINDOW,
		centeredX,
		centeredY,
		clientRect.right - clientRect.left,	// Calculated width
		clientRect.bottom - clientRect.top,	// Calculated height
		0,			// No parent window
		0,			// No menu
		hInstance,	// The app's handle
		0
		);

	if (fullscreen)
	{
		SetWindowLong(windowHandle, GWL_STYLE, 0);
	}

	ShowWindow(windowHandle, ShowWnd);
	UpdateWindow(windowHandle);
}

HWND Window::GetWindowHandle()
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

Window* Window::GetInstance()
{
	return Instance;
}

bool Window::IsFullscreen()
{
	return fullscreen;
}

SIZE Window::GetWindowSize()
{
	return SIZE{ width, height };
}

void Window::RegisterOnResizeCallback(std::function<void()> callback)
{
	onResizeCallbacks.push_back(callback);
}

Window::Window()
{
	Instance = this;
}

void Window::OnResize()
{
	for (auto& resizeCallback : onResizeCallbacks)
	{
		resizeCallback();
	}
}



// Window Exception Stuff
std::string Exception::TranslateErrorCode(HRESULT hardwareResult) noexcept
{
	char* pMsgBuf = nullptr;
	// windows will allocate memory for err string and make our pointer point to it
	const DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hardwareResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<wchar_t*>(&pMsgBuf), 0, nullptr
		);
	// 0 string length returned indicates a failure
	if (nMsgLen == 0)
	{
		return "Unidentified error code";
	}
	// copy error string from windows-allocated buffer to std::string
	std::string errorString = pMsgBuf;
	// free windows buffer
	LocalFree(pMsgBuf);
	return errorString;
}

HrException::HrException(int line, const char* file, HRESULT hardwareResult) noexcept
	:
	Exception(line, file),
	hardwareResult(hardwareResult)
{}

const char* HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
		<< "[Description] " << GetErrorDescription() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* HrException::GetType() const noexcept
{
	return "Window Exception";
}

HRESULT HrException::GetErrorCode() const noexcept
{
	return hardwareResult;
}

std::string HrException::GetErrorDescription() const noexcept
{
	return Exception::TranslateErrorCode(hardwareResult);
}

const char* NoGfxException::GetType() const noexcept
{
	return "Window Exception [No Graphics Engine]";
}