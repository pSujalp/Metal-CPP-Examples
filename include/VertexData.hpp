#pragma once

#include <simd/simd.h>

using namespace simd;

struct Vertex {
    float3 position;
    float3 normal;
    float2 textureCoordinate;
    int diffuseTextureIndex;
};

struct TextureInfo {
    int width;
    int height;
};

struct VertexData {
    float4 position;
    float4 normal;
};

struct TransformationData {
    float4x4 translationMatrix;
    float4x4 perspectiveMatrix;
};



struct VertexData
{
    // The [[position]] attribute of this member indicates that this value
    // is the clip space position of the vertex when this structure is
    // returned from the vertex function.
    float4 position [[position]];
    float4 normal;
};