#pragma once
#include "stdafx.h"

namespace custom
{
	class GraphicsContext;
}

class ID3D12App
{
public:
	// This function can be used to initialize application state and will run after essential
	// hardware resources are allocated.  Some state that does not depend on these resources
	// should still be initialized in the constructor such as pointers and flags.
	virtual void StartUp() = 0;
	virtual void CleanUp() = 0;

	// Decide if you want the app to exit.  
	virtual bool IsDone(); // called by Escape Key. // .cpp에서 ID3D12App::IsDone()으로서 정의.

	// The update method will be invoked once per frame.  Both state updating and scene
	// rendering should be handled by this method.
	virtual void Update(float deltaT) = 0;

	// Official rendering pass
	virtual void RenderScene() = 0;

	// Optional UI (overlay) rendering pass.  This is LDR.  The buffer is already cleared.
	virtual void RenderUI(class custom::GraphicsContext&) {};
};

namespace engine
{
	void Initialize(ID3D12App& app);
	void RunApplication(ID3D12App& app, const wchar_t* className);
}
#define MAIN_FUNCTION()  int wmain(/*int argc, wchar_t** argv*/)

#define CREATE_APPLICATION( app_class )              \
    MAIN_FUNCTION()                                  \
    {                                                \
        ID3D12App* app = new app_class();            \
        engine::RunApplication( *app, L#app_class ); \
        delete app;                                  \
        return 0;                                    \
    }