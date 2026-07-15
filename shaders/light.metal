#include <metal_stdlib>
using namespace metal;

struct LightVertexData {
    float4 position [[position]];
    float4 normal;
};

struct TransformationData {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 perspectiveMatrix;
};

struct LightFragmentOut {
    float4 color [[color(0)]];
    float4 mask  [[color(1)]];
};

vertex LightVertexData lightVertexShader(uint vertexID [[vertex_id]],
             constant LightVertexData* vertexData [[buffer(0)]],
             constant TransformationData* transformationData [[buffer(1)]])
{
    LightVertexData out = vertexData[vertexID];
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * vertexData[vertexID].position;
    return out;
}

fragment LightFragmentOut lightFragmentShader(LightVertexData in [[stage_in]]) {
    LightFragmentOut out;
    out.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    out.mask  = float4(1.0f, 0.0f, 0.0f, 1.0f); // also feeds the bloom pass
    return out;
}