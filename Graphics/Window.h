#pragma once
/*  
   App ¦£ Window ¦£ MyMouse
       ¦¢        ¦¦ MyKeyboard
	   ¦¢
	   ¦¢ Graphics
	   ¦¦
*/



class Window
{
public:
	Window();
	~Window();

	void				Initialize(HINSTANCE hInstance, int width, int height, const wchar_t* windowName = L"KSK", const wchar_t* windowTitle = L"D3D12_Engine", BOOL fullscreen = false, int ShowWnd = 1);
	HWND				GetWindowHandle() noexcept;
	void				StartMessagePump(std::function<void()> callback);
	static HINSTANCE    GetInstance() noexcept;
	BOOL				IsFullscreen() noexcept;
	SIZE			    GetWindowSize() noexcept;
	void				OnResize() noexcept;
	void				RegisterOnResizeCallback(std::function<void()> callback)  noexcept;

	void EnableCursor() noexcept; 
	void DisableCursor() noexcept;
	bool GetCursorEnabled() const noexcept;

private:
	static LRESULT CALLBACK handleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	static LRESULT CALLBACK handleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	LRESULT handleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void confineCursor() noexcept;
	void freeCursor() noexcept;
	void showCursor() noexcept;
	void hideCursor() noexcept;
	void enableImGuiMouse() noexcept;
	void disableImGuiMouse() noexcept;

private:
	static Window*      Instance;
	HINSTANCE           hInstance;
	HWND		        windowHandle;

	std::vector<std::function<void()>> onResizeCallbacks = {}; // Swapchain, Resize
	std::vector<byte> rawInputData;
	std::wstring	  windowName;
	std::wstring      windowTitle;

	MyKeyboard        myKeyboard;
	MyMouse           myMouse;

	int			      width;
	int			      height;
	BOOL		      fullscreen;
	BOOL              cursorEnabled = true;
	// rawinput data scpoed
};