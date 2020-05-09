// CODE
// (search for "[SECTION]" in the code to find them)
// Index of this file:
// [SECTION] Forward Declarations

#pragma once
#include <windows.h>
#include <functional>
#include <string>
#include "ExceptionMessage.h"

class HrException;

class Window
{
	friend class Renderer;
public:
	static Window*      Instance;
	LRESULT CALLBACK	WindowsMessageCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void				Initialize(HINSTANCE hInstance, int width, int height, const wchar_t* windowName = L"KSK", const wchar_t* windowTitle = L"D3D12_Engine", BOOL fullscreen = false, int ShowWnd = 1);
	HWND				GetWindowHandle() noexcept;
	void				StartMessagePump(std::function<void()> callback);
	static Window*      GetInstance() noexcept;
	bool				IsFullscreen() noexcept;
	SIZE			    GetWindowSize() noexcept;
	void				RegisterOnResizeCallback(std::function<void()> callback)  noexcept;
	void				OnResize() noexcept;
	~Window();
private:
	Window();
	std::vector<std::function<void()>> onResizeCallbacks = {};
	std::wstring	windowName;
	std::wstring    windowTitle;
	int			    width;
	int			    height;
	BOOL		    fullscreen;
	HWND		    windowHandle;
};

class Exception : public ExceptionMessage
{
	using ExceptionMessage::ExceptionMessage;
public:
	static std::string TranslateErrorCode(HRESULT hardwareResult) noexcept;
};

class HrException final : public Exception
{
public:
	HrException(int line, const char* file, HRESULT hardwareResult) noexcept;
	const char* what() const noexcept override;
	const char* GetType() const noexcept override;
	HRESULT GetErrorCode() const noexcept;
	std::string GetErrorDescription() const noexcept;
private:
	HRESULT hardwareResult;
};

class NoGfxException final : public Exception
{
public:
	using Exception::Exception;
	const char* GetType() const noexcept override;
};