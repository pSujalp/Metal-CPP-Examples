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

};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
             constant VertexDataPosition* vertexData[[buffer(0)]],
             constant VertexDataUV* vertexDataUV [[buffer(1)]],
             constant TransformationData* transformationData [[buffer(2)]])
{
    VertexOut out;
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * float4(vertexData[vertexID].position,1.0f);
    out.textureCoordinate = vertexDataUV[vertexID].textureCoordinate;
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    
    
    const float4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);
    return colorSample ;

    // return float4(0.0f,1.0f,0.0f,1.0f);
}