
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


struct N_Uniforms{
     float3 lightPos;
     float3 viewPos;
     float3x3 normalMatrix;
};

struct NVertexData {
	float3 Position;
	float3 Normal;
    float2 TexCoords;
	float3 Tangent;
	float3 Bitangent;
};


struct N_MVP{
    matrix_float4x4 M;
    matrix_float4x4 V;
    matrix_float4x4 P;
};