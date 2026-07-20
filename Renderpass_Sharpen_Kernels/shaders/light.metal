//
//  light.metal
//  Metal-Tutorial
//

#include <metal_stdlib>

using namespace metal;



struct LightVertexData{
    float4 position [[position]];
    float4 normal;
};


struct TransformationData {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 perspectiveMatrix;
};

vertex LightVertexData lightVertexShader(uint vertexID [[vertex_id]],
             constant LightVertexData* vertexData [[ buffer(0) ]],
             constant TransformationData* transformationData[[ buffer(1) ]])
{
    LightVertexData out = vertexData[vertexID];
    
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * vertexData[vertexID].position;
    return out;
}

fragment float4 lightFragmentShader(LightVertexData in [[stage_in]],
                                    constant float4& lightColor [[ buffer(0) ]]) {
    return lightColor;
}