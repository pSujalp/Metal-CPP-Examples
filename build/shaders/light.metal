//
//  light.metal
//  Metal-Tutorial
//

#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float4 position;
    float4 normal;
};

struct VertexOut {
    float4 position [[position]];
    float4 normal;
};

vertex VertexOut lightVertexShader(
    uint vertexID                        [[vertex_id]],
    constant VertexIn*    vertexData     [[buffer(0)]],
    constant float4x4&    modelMatrix    [[buffer(1)]],
    constant float4x4&    perspectiveMatrix [[buffer(2)]])
{
    VertexOut out;
    out.position = perspectiveMatrix * modelMatrix * vertexData[vertexID].position;
    out.normal   = vertexData[vertexID].normal;
    return out;
}

fragment float4 lightFragmentShader(
    VertexOut           in         [[stage_in]],
    constant float4&    lightColor [[buffer(0)]])
{
    return lightColor;
}
