#pragma once
/*
   (Global) ReadyMadePSO

   App ¦£ Window
	   ¦¢ Graphics   
	   ¦¢
	   ¦¢ 
	   ¦¦
*/

// will be called at main function.
// int CALLBACK WinMain(
// HINSTANCE hInstance,
// HINSTANCE hPrevInstance,
// LPSTR     lpCmdLine,
// int       nCmdShow )
class App
{
public:
	App(const wchar_t* argv);
	BOOL InitializeApplication(void); // Init All Component.
	BOOL TerminateApplication(void);
	BOOL RunApplication();

private:
	BOOL UpdateApplication(float deltaTime);
	BOOL RenderGraphics(void);
	BOOL RenderWindowUI() {};

private:
	Window WND;
	Graphics GRPX;
};

/*
class App
{
private:
	Window
	Graphics
	Camera µÎ°³
	ViewPort->DirectX::Matrix4x4, D3D12_VIEWPORT
	D3D12_RECT m_MainScissor;
	ModelManager
}

*/
