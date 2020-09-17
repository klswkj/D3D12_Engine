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

// MainLight �޾ƾߵ�
class MainPass : public Pass
{
	MainPass(const char* pName);

	void Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT override;
	void Reset() DEBUG_EXCEPT override;
	// Binding�ȰŴ� �Һ�(Immutable)�ؾ��ϴ°� ���

private:
	// std::vector<Job> m_Jobs; At RenderQueuePass
	// Job { pDrawable, Step(vector<RenderingResource>) } -> Draw�ѹ��ϱ� ���� ����
};