#pragma once
#include "FilledPrimitiveSphere.h"
#include "ShadowCamera.h"

class MainLight
{

private:
	FilledPrimitiveSphere m_Mesh;
	ShadowCamera m_ShadowCamera;
};

