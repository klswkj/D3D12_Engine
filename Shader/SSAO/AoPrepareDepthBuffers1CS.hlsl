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

Texture2D<float> Depth            : register(t0);
RWTexture2D<float> LinearZ        : register(u0); // LinearDepth
RWTexture2D<float2> DS2x          : register(u1); // g_DepthDownsize1 BufferSize / 2
RWTexture2D<float2> DS4x          : register(u2); // g_DepthDownsize2 BufferSize / 4
RWTexture2DArray<float> DS2xAtlas : register(u3); // g_DepthTiled1    BufferSize / 8
RWTexture2DArray<float> DS4xAtlas : register(u4); // g_DepthTiled2    BufferSize / 16

cbuffer CB0 : register(b0)
{
    float ZMagic;
}

float Linearize( uint2 st )
{
    float depth = Depth[st];
    float dist = 1.0 / (ZMagic * depth + 1.0);
    LinearZ[st] = dist;
    return dist;
}

groupshared float g_CacheW[256];

[RootSignature(SSAO_RootSig)]
[numthreads( 8, 8, 1 )] // ThreadCount (0 ~ 7,    0 ~ 7)
                        // GroupCount  (0 ~ 119, 0 ~ 66)
//                       1st                      2nd                          3rd                       4th                   
void main(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex)
{
    uint2 startST = Gid.xy << 4 | GTid.xy;  // Gid => (0~1904, 0 ~ 1056) |(imad) GTid => (0 ~ 7, 0 ~ 7)
    uint destIdx  = GTid.y << 4 | GTid.x;    // GTid=> (0 ~ 112) |(+) (0~8) => (0 ~ 120)
    g_CacheW[ destIdx +  0  ] = Linearize(startST | uint2(0, 0)); // g_CacheW[ 0 ~ 40 ] = Linear(
    g_CacheW[ destIdx +  8  ] = Linearize(startST | uint2(8, 0));
    g_CacheW[ destIdx + 128 ] = Linearize(startST | uint2(0, 8));
    g_CacheW[ destIdx + 136 ] = Linearize(startST | uint2(8, 8));

    GroupMemoryBarrierWithGroupSync();

    // l? + downSamplingIndex
    uint ldsIndex = (GTid.x << 1) | (GTid.y << 5);

    float w1 = g_CacheW[ldsIndex];

    uint2 st = DTid.xy;
    uint slice = (st.x & 3) | ((st.y & 3) << 2);
    DS2x[st] = w1;
    DS2xAtlas[uint3(st >> 2, slice)] = w1;

    // if ((GI & 0x1011) == 0)
    if ((GI & 011) == 0)
    {
        st = DTid.xy >> 1;
        slice = (st.x & 3) | ((st.y & 3) << 2);
        DS4x[st] = w1;
        DS4xAtlas[uint3(st >> 2, slice)] = w1;
    }
}

/*
//                       1st                      4th                         2nd                        3rd                   
void main( uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
    uint2 startST = Gid.xy << 4 | GTid.xy;  // Gid => (0~120, 0~67) << 4 | GTid => (0~8, 0~8)
    uint destIdx = GTid.y << 4 | GTid.x;    // GTid=> (0~8) << 4 | (0~8)
    g_CacheW[ destIdx +  0  ] = Linearize(startST | uint2(0, 0));
    g_CacheW[ destIdx +  8  ] = Linearize(startST | uint2(8, 0));
    g_CacheW[ destIdx + 128 ] = Linearize(startST | uint2(0, 8));
    g_CacheW[ destIdx + 136 ] = Linearize(startST | uint2(8, 8));

    GroupMemoryBarrierWithGroupSync();

    uint ldsIndex = (GTid.x << 1) | (GTid.y << 5);

    float w1 = g_CacheW[ldsIndex];

    uint2 st = DTid.xy;
    uint slice = (st.x & 3) | ((st.y & 3) << 2);
    DS2x[st] = w1;
    DS2xAtlas[uint3(st >> 2, slice)] = w1;

    if ((GI & 011) == 0)
    {
        st = DTid.xy >> 1;
        slice = (st.x & 3) | ((st.y & 3) << 2);
        DS4x[st] = w1;
        DS4xAtlas[uint3(st >> 2, slice)] = w1;
    }
}
*/

