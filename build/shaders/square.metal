
#include <metal_stdlib>
using namespace metal;

#include <simd/simd.h>

using namespace simd;

struct VertexData {
    float4 position;
    float2 textureCoordinate;
};

struct Uniforms
{
    float2 time;
};


struct Uniforms1
{
    int intAsBool;
};

struct MVP{
    matrix_float4x4 MVP;
};



struct VertexOut {
    float4 position [[position]];
    float2 textureCoordinate;
};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                              constant VertexData* vertexData,
                              constant Uniforms& uniforms [[buffer(1)]],
                               constant Uniforms1& uniforms1 [[buffer(2)]],
                               constant MVP& mvp [[buffer(3)]]) {
    VertexOut out;
    if(uniforms1.intAsBool){
        out.position = vertexData[vertexID].position + float4(uniforms.time,0,0);
    }
    else out.position = vertexData[vertexID].position ;

    out.position = mvp.MVP * out.position;
    out.textureCoordinate = vertexData[vertexID].textureCoordinate;
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);

    float4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);

    if(colorSample.a < 0.1) {
        discard_fragment();
    }
    colorSample.a= 0.5f;
    return colorSample;
}
