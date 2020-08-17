#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <DirectXMath.h>

class Pass;

class PassWindow
{
public:
	PassWindow(const char* name)
		: m_Name(name)
	{
	}

	void RegisterPass(Pass* pass);
	void RenderPassWindow();

private:
	void listAllPasses();

private:
	const char* m_Name;
	std::vector<Pass*> m_Passes;
	Pass* m_pSelectedPass;
	int32_t m_SelectedPassIndex;
};

// Pass측에 Finalized하면, PassWindow.cpp나 아니면 여기 위에 있는
// std::vector<Pass*>에 저장되게.