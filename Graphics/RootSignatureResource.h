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
// RootSignatureResource : public RootSignauture, public RenderingResource
// PSOResource           : public PSO,            public RenderingResource
// CommnadContextResource: public custom          public RenderingResource

// 현재 이슈 : RootSignatureResource에서 
// ReserveSomething으로 차례대로 Reserve할껀데 Multiple Inheritance로 인한, RenderingResource*로 컨테이너에 한번에 묶으면
// 정상적으로 저장될지, 그리고 컨테이너에 저장했으면 각각, RootSignatureResource, PSOResource, CommandContextResource
*/
/*
시작
GraphicsCommandContext::Begin();

끝
g_ContextManager.FreeContext(this[CommandContext]);
*/

// 

class RootSignatureResource : public RenderingResource // Bindable
{

};