
#pragma once
#include <simd/simd.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

using namespace simd;


struct MVP
{
    matrix_float4x4 M;
    matrix_float4x4 V;
    matrix_float4x4 P;
    matrix_float3x3 Normal;
};



struct MVP_skybox{
    matrix_float4x4 MVP;

};


struct Uniforms
{
    float2 time;
};


struct VertexData {
    float3 position;
    float3 normal;
};

struct Camera{
    float3 cameraPosition;
};
