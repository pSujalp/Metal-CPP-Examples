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

    float3 hdrColor = colorTexture.sample(textureSampler, in.textureCoordinate).rgb;

    // Exposure tonemap (Reinhard-style exponential). This only does something
    // meaningful now that the offscreen render target is a floating-point
    // format (RGBA16Float) and can actually hold values above 1.0.
    float3 result = float3(1.0f) - exp(-hdrColor * 1.9f);

    // No manual pow(1/2.2) gamma correction here: the final pipeline's
    // color attachment is BGRA8Unorm_sRGB, which gamma-encodes on write
    // automatically. Doing it manually too would double gamma-correct
    // and wash the image out.

    return float4(result, 1.0f);
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
