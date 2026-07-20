//
//  VertexData.h
//  Metal-Tutorial
//

#pragma once
#include <simd/simd.h>

using namespace simd;

struct VertexData {
    float4 position;
    float2 textureCoordinate;
    float3 color;
};


struct Uniforms
{
    float2 time;
    int intAsBool;
};


struct Uniforms1
{
    int intAsBool;
};
