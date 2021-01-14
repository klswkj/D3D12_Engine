#pragma once
#include "Model.h"
#include "ModelComponentWindow.h"

class MasterRenderGraph;
enum class eObjectFilterFlag;

class ObjModelManager
{
public:
	ObjModelManager(float ModelSize = 1.0f);
	~ObjModelManager();
	void LinkTechniques(MasterRenderGraph& _MasterRenderGraph);
	void Submit(eObjectFilterFlag Filter);
	void RenderComponentWindows();
	void ComputeBoundingBox();

	struct BoundingBox
	{
		Math::Vector3 MinPoint = Math::Scalar(FLT_MAX);
		Math::Vector3 MaxPoint = Math::Scalar(FLT_MIN);
	};
	BoundingBox& GetBoundingBox();

private:
	size_t                            m_ActiveModelIndex;
	BoundingBox                       m_BoundingBox;
	std::vector<Model>                m_Models;
	std::vector<ModelComponentWindow> m_ModelComponentWindows;
};