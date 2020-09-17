#pragma once
#include "Pass.h"
#include "Matrix4.h"
#include "ShaderConstantsTypeDefinitions.h"
namespace custom
{
	class CommandContext;
}

class IBaseCamera;
class ShadowCamera;

// MainLight 받아야됨
class MainPass : public Pass
{
	MainPass(const char* pName);

	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;
	// Binding된거는 불변(Immutable)해야하는거 명심

private:
	// std::vector<Job> m_Jobs; At RenderQueuePass
	// Job { pDrawable, Step(vector<RenderingResource>) } -> Draw한번하기 위한 단위
};