#include "stdafx.h"
#include "Main.h"

// [Source Code Navigator] 
// 
// Global range :
// 
// 1. Windows   : win32, Input
// 2. Device    : Device, SwapChain, ...
// 3. Graphics  : PSO, RootSignature, Sampler, CommandManager, CommandContext
// 
// class : Model
//

class KSKApp : public ID3D12App
{
public:
	KSKApp() {}

	virtual void StartUp() override;
	virtual void CleanUp() override;

	virtual void Update(float deltaT) override;
	virtual void RenderScene(void) override;

	void RenderUI(class custom::GraphicsContext&) override;


};