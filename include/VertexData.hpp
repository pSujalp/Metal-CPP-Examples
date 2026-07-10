
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


struct NVertexData{
    float3 position;
    float3 TexCoords;
    float3 normal;
    float3 tangent;
    float3 bitangent;

};

struct N_MVP{
    matrix_float4x4 MVP;
};