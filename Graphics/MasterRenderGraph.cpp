#include "stdafx.h"
#include "MasterRenderGraph.h"

#include "ShadowBuffer.h"
#include "ObjectFilterFlag.h"
#include "Matrix4.h"
#include "ShaderConstantsTypeDefinitions.h"

#include "Camera.h"
#include "RenderTarget.h"
#include "DepthStencil.h"

#include "Pass.h"
#include "ShadowMappingPass.h"
#include "LightPrePass.h"
#include "SSAOPass.h"

/*
�ϴ� ó�� ���� ���� Pass�� ���� ���� �ѹ��� Shadow Camera, MainCamera �����ϰ� �ѹ�������, ������Ʈ�ϰ� �ѹ� ������ �ϴ� ���̸�,

// MainLightManager����� �����ڿ� std::vector<Math::vector3>& �����̳� ���۷��� ���� Pass �ȿ��� ���� .size()��ŭ ������
*/

MasterRenderGraph::MasterRenderGraph()
//: m_pRenderTarget(), m_pMasterDepth()
{
	

	finalize();
}

MasterRenderGraph::~MasterRenderGraph()
{
}

// Pass windowâ �����ְ�, ���ϴ� Pass�� Enable->Disable
void MasterRenderGraph::Execute(custom::CommandContext& BaseContext) DEBUG_EXCEPT
{
	assert(finalized);

	for (auto& p : m_pPasses)
	{
		if (p->m_bActive)
		{
			p->Execute(BaseContext);
		}
	}
}

void MasterRenderGraph::Reset() noexcept
{
	assert(finalized);
	for (auto& p : m_pPasses)
	{
		p->Reset();
	}
}

RenderQueuePass& MasterRenderGraph::GetRenderQueue(const std::string& passName)
{
	for (const auto& p : m_pPasses)
	{
		if (p->GetRegisteredName() == passName)
		{
			return dynamic_cast<RenderQueuePass&>(*p);
		}
	}
}

void MasterRenderGraph::ShowPassesWindows()
{

}

void MasterRenderGraph::BindMainCamera(Camera& cam)
{
	/*
	dynamic_cast<LambertianPass&>(findPassByName("lambertian")).BindMainCamera(cam);
	*/
}

void MasterRenderGraph::BindShadowCamera(Camera& cam)
{
	/*
	dynamic_cast<ShadowMappingPass&>(findPassByName("shadowMap")).BindShadowCamera(cam);
	dynamic_cast<LambertianPass&>(findPassByName("lambertian")).BindShadowCamera(cam);
	*/
}

void MasterRenderGraph::finalize()
{
	assert(!finalized);
	for (const auto& p : m_pPasses)
	{
		p->finalize();
	}

	finalized = true;
}

void MasterRenderGraph::appendPass(std::unique_ptr<Pass> pass)
{
	assert(!finalized);
	// validate name uniqueness
	for (const auto& p : m_pPasses)
	{
		if (pass->GetRegisteredName() == p->GetRegisteredName())
		{
			ASSERT(false, "Pass name already exists : ", pass->GetRegisteredName())
		}
	}

	// Add to container of passes
	m_pPasses.push_back(std::move(pass));
}

Pass& MasterRenderGraph::findPassByName(const std::string& name)
{
	const auto i = std::find_if(m_pPasses.begin(), m_pPasses.end(), 
		[&name](auto& p) { return p->GetRegisteredName() == name; });

	ASSERT(i == m_pPasses.end(), "Failed to find pass name");

	return **i;
}