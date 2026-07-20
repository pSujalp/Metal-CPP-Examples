
#include <metal_stdlib>
using namespace metal;

#include <simd/simd.h>

using namespace simd;

struct VertexData {
    float4 position;
    float2 textureCoordinate;
    float3 color;
};

struct Uniforms
{
    float2 time;
};


struct Uniforms1
{
    int intAsBool;
};



struct VertexOut {
    float4 position [[position]];
    float2 textureCoordinate;
    float3 color;
};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                              constant VertexData* vertexData,
                              constant Uniforms& uniforms [[buffer(1)]],
                               constant Uniforms1& uniforms1 [[buffer(2)]]) {
    VertexOut out;
    if(uniforms1.intAsBool){
        out.position = vertexData[vertexID].position + float4(uniforms.time,0,0);
    }
    else out.position = vertexData[vertexID].position ;
    out.textureCoordinate = vertexData[vertexID].textureCoordinate;
    out.color =vertexData[vertexID].color;
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]],
                               texture2d<float> colorTexture1 [[texture(1)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    const float4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);
    const float4 colorSample1 = colorTexture1.sample(textureSampler, in.textureCoordinate);

    return mix(colorSample,colorSample1,0.10f) * float4(in.color,1.0f);
}
