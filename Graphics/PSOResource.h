#pragma once
#include "RenderingResource.h"
/*
//  RootSignature, PSO, CommandContext 에 각각 이름 붙이기
//                                        RenderingResource
//                                                │
//                    ┌───────────────────────────┼───────────────────────────┐
//              RootSignature                    PSO                    CommandContext
//                 Resource                    Resource                   Resource
//              1. Sampler                  1. RootSignature           1. RenderTarget
//           2. RootParameter               2. VS                      2. DepthStencil
//        2-1 ~ 3. CBV, SRV, UAV            3. PS                      3. VertexBuffer
// 2.4, 5. DescriptorTable, 32Bit Constant  4. Stencil                 4. IndexBuffer
//                                          5. Topology
//                                          6. Blend
//                                          7. Rasterizer
//                                    더추가 : Depth_Stencil_Desc
//                                          : InputLayout
//
// RootSignatureResource : public RootSignature, public RenderingResource

// PSOResource           : public PSO,           public RenderingResource

// CommnadContextResource: public custom         public RenderingResource

*/

/*
시작
GraphicsCommandContext::Begin();

끝
g_ContextManager.FreeContext(this[CommandContext]);
*/

// 이것도 PSOResource 클래스 따로 필요없고,
// 구별은, VisualStudio내에서만 Filter로만할까...? 고민되네
// 그리고 세밀하게, (대충 루트시그니쳐) : public RootSignature, public RenderingResource 면,
// 

// 아니다 이거 PSOResource를 순수가상함수로 만들고,
// 아래 클래스들한테, 내가 PSO 구조체 내에서 해당하는 부분에 있으면 에러내고, 
// 없으면 나의 부분 복사하게 함수 하나 만들자.
// 원래의 D3D12_GRAPHICS_PIPELINE_STATE_DESC의 개체는 Pass에 속해있음.
// 그냥 PSOReousrce 필요 없을듯.

class PSOResource : public RenderingResource
{
private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc;
};