
#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;
using namespace simd;



struct MVP
{
    matrix_float4x4 M;
    matrix_float4x4 V;
    matrix_float4x4 P;
    matrix_float3x3 Normal;
};

struct Uniforms
{
    float2 time;
};

struct VertexData {
    float3 position;
    float3 normal;
};

struct Camera{
    float3 cameraPosition;
};

struct VertexOut {

    float4 position [[position]];
    float3 Position;
    float3 Normal;
};
vertex VertexOut vertexShader(
    uint vertexID [[vertex_id]],
    constant VertexData* vertexData [[buffer(0)]],
    constant Uniforms& uniforms [[buffer(1)]],
    constant MVP& mvp [[buffer(2)]])
{
    VertexOut out;

    float4 worldPos = mvp.M * float4(vertexData[vertexID].position, 1.0);

    out.position = mvp.P * mvp.V * worldPos;
    out.Position = worldPos.xyz;


    out.Normal = normalize(mvp.Normal * vertexData[vertexID].normal);

    return out;
}
float2 SampleSphericalMap(float3 v)
{
    float2 uv;
    uv.x = atan2(v.z, v.x) / (2.0f * M_PI_F) + 0.5f;
    uv.y = asin(v.y) / M_PI_F + 0.5f;
    return uv;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               constant Camera & camera [[buffer(0)]],
                               texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);

    float3 I = normalize(in.Position - camera.cameraPosition);
    float3 R = reflect(I, normalize(in.Normal));
    
    float2 uv = SampleSphericalMap(normalize(R));
    float4 color = colorTexture.sample(textureSampler, uv);

     
    return color;
}
