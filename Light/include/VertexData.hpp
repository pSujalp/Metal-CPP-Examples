#pragma once
#include <simd/simd.h>

using namespace simd;



struct LightVertexData{
    float4 position [[position]];
    float4 normal;
};
struct VertexData {
    float4 position;
    float4 normal;
};

struct TransformationData {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 perspectiveMatrix;
};

struct OutData {
    float4 position [[position]];
    float4 normal;
    float4 fragmentPosition;
};
