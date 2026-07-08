//
//  VertexData.h
//  Metal-Tutorial
//

#pragma once
#include <simd/simd.h>

using namespace simd;

struct VertexIn
{
    float4 position;
    float4 color;
};

struct VertexOut
{
    float4 position ;
    float4 color;
};
