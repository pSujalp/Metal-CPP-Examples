
#pragma once
#include <simd/simd.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

using namespace simd;

struct VertexData {
    float4 position;
    float2 textureCoordinate;
};

struct MVP{
    matrix_float4x4 MVP;
};

struct Uniforms
{   float2 time;
    int intAsBool;
};
