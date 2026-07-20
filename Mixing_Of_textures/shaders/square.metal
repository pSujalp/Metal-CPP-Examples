
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



struct VertexOut {
    // The [[position]] attribute of this member indicates that this value
    // is the clip space position of the vertex when this structure is
    // returned from the vertex function.
    float4 position [[position]];

    // Since this member does not have a special attribute, the rasterizer
    // interpolates its value with the values of the other triangle vertices
    // and then passes the interpolated value to the fragment shader for each
    // fragment in the triangle.
    float2 textureCoordinate;
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
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]],
                               texture2d<float> colorTexture1 [[texture(1)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    const float4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);
    const float4 colorSample1 = colorTexture1.sample(textureSampler, in.textureCoordinate);
    return mix(colorSample,colorSample1,0.10f);
}
