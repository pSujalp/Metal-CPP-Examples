
#include <metal_stdlib>
using namespace metal;

#include <simd/simd.h>

using namespace simd;



struct VertexData {
    float4 position;
    float2 textureCoordinate;
};

struct MVP{
    matrix_float4x4 MVP;
};

struct VertexOut {
    float4 position [[position]];
    float2 textureCoordinate;
};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                              uint instanceId [[instance_id]] ,
                              device const VertexData* vertexData [[buffer(0)]],
                              device const MVP * mvp [[buffer(1)]]) {
        VertexOut out;
        out.position = mvp[instanceId].MVP * vertexData[vertexID].position;
        out.textureCoordinate = vertexData[vertexID].textureCoordinate;
        return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    const float4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);
    return colorSample;
}
