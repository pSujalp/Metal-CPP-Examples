#include <metal_stdlib>
using namespace metal;
#include <simd/simd.h>
using namespace simd;

struct VertexData {
    float4 position;
    float2 textureCoordinate;
};

struct MVP {
    matrix_float4x4 modelViewProjection;
};

struct Uniforms {
    float aspectRatio;
};

struct AAPLVertex {
    float2 position;
    float4 color;
    float2 textureCoordinate;
   
};

struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
    float2 textureCoordinate;
};


struct AAPLOut {
    float4 position [[position]];
    float4 color;
    float2 textureCoordinate;
};

vertex AAPLOut vertexRenderPass(uint vertexID [[vertex_id]],
                                constant AAPLVertex* vertexData [[buffer(0)]]) {
    AAPLOut out;
    out.position = float4(vertexData[vertexID].position, 0.0, 1.0);
    out.color = vertexData[vertexID].color;
    out.textureCoordinate = vertexData[vertexID].textureCoordinate;
    return out;
}

fragment float4 fragmentRenderPass(AAPLOut in [[stage_in]],
                                   texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
            
    float4 result = colorTexture.sample(textureSampler, in.textureCoordinate);

    float gamma = 2.2f;
    result.rgb = pow(result.rgb, float3(1.0f/gamma));

    


    return result;
}
vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                              constant VertexData* vertexData [[buffer(0)]],
                              constant MVP& mvp [[buffer(1)]]) {
    VertexOut out;
    out.position = mvp.modelViewProjection * vertexData[vertexID].position;
    out.textureCoordinate = vertexData[vertexID].textureCoordinate;
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    float4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);


    return colorSample;
}
