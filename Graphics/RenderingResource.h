#pragma once
#include "stdafx.h"

/*
//  RootSignature, PSO, CommandContext 에 각각 이름 붙이기
//                                                   RenderingResource
//                                                           │
//             ┌─────────────────────────────────────────────┼───────────────────────────┐
//        RootSignature                                     PSO                    CommandContext
//           Resource                                     Resource                   Resource
//         1. Sampler                                 1. RootSignature           1. RenderTarget
//         2. RootParameter                           2. VS                      2. DepthStencil
//         2-1 ~ 3. CBV, SRV, UAV                     3. PS                      3. VertexBuffer
//         2.4, 5. DescriptorTable, 32Bit Constant    4. Stencil                 4. IndexBuffer
//                                                    5. Topology
//                                                    6. Blend
//                                                    7. Rasterizer
//                                                    8. Depth_Stencil_Desc
//                                                    9. InputLayout
//

// RootSignatureResource : public GPUBuffer,     public RenderingResource

// PSOResource           : public PSO,           public RenderingResource

// CommnadContextResource: public custom         public RenderingResource
*/


/*
class Entity;

namespace custom
{
	class CommandContext;
}

class RenderingResource // Bindable
{
public:
	virtual ~RenderingResource() = default;
	virtual void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT = 0;
	virtual void InitializeParentReference(const Entity&) noexcept = 0;

	virtual std::string GetUID() const noexcept = 0;
};

class CloningPtr : public RenderingResource
{
public:
	virtual std::unique_ptr<CloningPtr> Clone() const noexcept = 0;
};



class X
{
public:
  shared_ptr<X> Clone() const
  {
	return CloneImpl();
  }
private:
  virtual shared_ptr<X> CloneImpl() const
  {
	return(shared_ptr<X>(new X(*this)));
  }
};

class Y : public X
{
public:
  shared_ptr<Y> Clone() const
  {
	return(static_pointer_cast<Y, X>(CloneImpl())); // no need for dynamic_pointer_cast
  }
private:
  virtual shared_ptr<X> CloneImpl() const
  {
	return shared_ptr<Y>(new Y(*this));
  }
};
*/

class Entity;

namespace custom
{
	class CommandContext;
}

class RenderingResource // Bindable
{
public:
	virtual ~RenderingResource() = default;
	virtual void Bind(custom::CommandContext& BaseContext) DEBUG_EXCEPT = 0;
	// virtual void InitializeParentReference(const Entity&) noexcept = 0;
	// virtual std::string GetUID() const noexcept = 0;
};

class CloningPtr : public RenderingResource
{
public:
	virtual std::unique_ptr<CloningPtr> Clone() const noexcept = 0;
};