//
//  cube.metal
//  MetalTutorial
//

#include <metal_stdlib>
using namespace metal;

#include "VertexData.hpp"

struct VertexOut {

    float4 position [[position]];
    float2 textureCoordinate;
    float3 normal;
};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
             constant VertexData* vertexData,
             constant TransformationData* transformationData)
{
    VertexOut out;
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * vertexData[vertexID].position;
    out.textureCoordinate = vertexData[vertexID].textureCoordinate;
    out.normal = vertexData[vertexID].normal;
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    
    
    float3 L = normalize(float3(1, 1, 1));
    float3 N = normalize(in.normal);
    float NdotL = saturate(dot(N, L));


    return float4(float3(NdotL), 1) * float4(1.0f,0.0f,0.0f,1.0f);
}