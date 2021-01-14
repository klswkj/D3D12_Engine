//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "SSAORS.hlsli"

RWTexture2D<float> LinearZ : register(u0);
Texture2D<float>   Depth   : register(t0);

cbuffer CB0 : register(b0)
{
    float ZMagic;                // (zFar - zNear) / zNear => (1000.0f - 1.0f) / 1.0f
}

// Depth => 0.0f ~ 1.0f ZLocation / ZFar

[RootSignature(SSAO_RootSig)]
[numthreads( 16, 16, 1 )]
void main( uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
    LinearZ[DTid.xy] = 1.0 / (ZMagic * Depth[DTid.xy] + 1.0);
    // LinearZ[DTid.xy] = 1.0f / ( ((ZFar - ZNear) / zNear) * Depth
    //                  = 1.0f / (ZFar - ZNar) * ZLocation / ZNear * ZNear => ZLocation * (1 / ZNear - 1 / ZFar) + 1.0f
    //                  = 1.0f / ZLocation * ( 1 / ZNear - 1 / ZFar + 1 / ZLocation)
}
