#pragma once
#include "RenderingResource.h"
/*
//  RootSignature, PSO, CommandContext �� ���� �̸� ���̱�
//                                        RenderingResource
//                                                ��
//                    ������������������������������������������������������������������������������������������������������������������
//              RootSignature                    PSO                    CommandContext
//                 Resource                    Resource                   Resource
//              1. Sampler                  1. RootSignature           1. RenderTarget
//           2. RootParameter               2. VS                      2. DepthStencil
//        2-1 ~ 3. CBV, SRV, UAV            3. PS                      3. VertexBuffer
// 2.4, 5. DescriptorTable, 32Bit Constant  4. Stencil                 4. IndexBuffer
//                                          5. Topology
//                                          6. Blend
//                                          7. Rasterizer
//                                    ���߰� : Depth_Stencil_Desc
//                                          : InputLayout
//
// RootSignatureResource : public RootSignature, public RenderingResource

// PSOResource           : public PSO,           public RenderingResource

// CommnadContextResource: public custom         public RenderingResource
*/

/*
����
GraphicsCommandContext::Begin();

��
g_ContextManager.FreeContext(this[CommandContext]);
*/

// �ʿ��Ѱ� GraphicsCommandContext
// ���� CommandContextResource�� �ʿ���� �� ����.

class CommandContextResource : public RenderingResource
{
public:
	CommandContextResource();
	virtual ~CommandContextResource();

	void Bind();
	void InitializeParentReference();

};