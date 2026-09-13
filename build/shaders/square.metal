#include <metal_stdlib>
using namespace metal;
#include <simd/simd.h>
using namespace simd;

struct Vertex
{
    float3 position;
    float3 normal;
    float3 tangent;
    float3 bitangent;
    float2 textureCoordinate;
    int    diffuseTextureIndex;
    int    specularTextureIndex;
    int    normalMapIndex;
    int    emissiveMapIndex;
};

struct VertexOut
{
    float4 position [[position]];
    float2 textureCoordinate;
    float4 shadowPosition;
    int    diffuseTextureIndex;
};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                               constant Vertex *vertexData,
                               constant float4x4 &modelMatrix              [[buffer(1)]],
                               constant float4x4 &viewMatrix                [[buffer(2)]],
                               constant float4x4 &projectionMatrix          [[buffer(3)]],
                               constant float4x4 &lightViewProjectionMatrix [[buffer(4)]])
{
    VertexOut out;
    Vertex v = vertexData[vertexID];
    float4 worldPosition = modelMatrix * float4(v.position, 1.0);

    out.position             = projectionMatrix * viewMatrix * worldPosition;
    out.textureCoordinate    = v.textureCoordinate;
    out.shadowPosition       = lightViewProjectionMatrix * worldPosition;
    out.diffuseTextureIndex  = v.diffuseTextureIndex;
    return out;
}

fragment float4 TexturefragmentShader(VertexOut in [[stage_in]],
                                       texture2d_array<float> colorTextures [[texture(3)]],
                                       depth2d<float>         shadowMap     [[texture(4)]])
{
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    constexpr sampler shadowSampler(coord::normalized,
                                     filter::linear,
                                     address::clamp_to_edge,
                                     compare_func::less_equal);

    float4 colorSample = colorTextures.sample(textureSampler, in.textureCoordinate, in.diffuseTextureIndex);

    float3 shadowNDC = in.shadowPosition.xyz / in.shadowPosition.w;
    float2 shadowUV  = shadowNDC.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    float bias = 0.0015;
    float currentDepth = shadowNDC.z - bias;

    float lit = 1.0;
    if (shadowUV.x >= 0.0 && shadowUV.x <= 1.0 &&
        shadowUV.y >= 0.0 && shadowUV.y <= 1.0 &&
        currentDepth <= 1.0)
    {
        lit = shadowMap.sample_compare(shadowSampler, shadowUV, currentDepth);
    }

    float shadowFactor = mix(0.35, 1.0, lit);
    return float4(colorSample.rgb * shadowFactor, colorSample.a);
}

vertex float4 vertex_zOnly(uint vertexID [[vertex_id]],
                           constant Vertex *vertexData,
                           constant float4x4 &modelMatrix              [[buffer(1)]],
                           constant float4x4 &lightViewProjectionMatrix [[buffer(2)]])
{
    float4 worldPosition = modelMatrix * float4(vertexData[vertexID].position, 1.0);
    return lightViewProjectionMatrix * worldPosition;
}