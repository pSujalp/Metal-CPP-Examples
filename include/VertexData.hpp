//
//  VertexData.h
//  Metal-Tutorial
//

#pragma once
#include <simd/simd.h>

using namespace simd;

struct VertexDataPosition {
    float3 position;
};

struct VertexDataUV {
    float2 textureCoordinate;
};
struct VertexDataNormal {
    float3 normal;
};


struct TransformationData {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 perspectiveMatrix;
    float3x3 normalMatrix;
};


struct Uniforms{

    float3 cameraPosition;
    float3 lightPosition;
    float3 lightColor;
    float3 albedo;
    float metallic;
    float roughness;
    float ao;
};

