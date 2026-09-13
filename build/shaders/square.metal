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
    float3 worldPosition;      // NEW: needed for correct lighting/view vectors
    float2 textureCoordinate;
    float4 shadowPosition;
    int    diffuseTextureIndex;
    float3 normal;
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
    out.worldPosition        = worldPosition.xyz;
    out.textureCoordinate    = v.textureCoordinate;
    out.shadowPosition       = lightViewProjectionMatrix * worldPosition;
    out.diffuseTextureIndex  = v.diffuseTextureIndex;

    // NOTE: if modelMatrix ever has non-uniform scale, transform normals with
    // the inverse-transpose of the upper-left 3x3 of modelMatrix instead of
    // rotating them directly, to avoid skewed normals.
    out.normal = normalize((modelMatrix * float4(v.normal, 0.0)).xyz);

    return out;
}

fragment float4 TexturefragmentShader(VertexOut in [[stage_in]],
                                       texture2d_array<float> colorTextures [[texture(3)]],
                                       depth2d<float>         shadowMap     [[texture(4)]],
                                       constant float4        &viewPos     [[buffer(5)]])
{
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    constexpr sampler shadowSampler(coord::normalized,
                                     filter::linear,
                                     address::clamp_to_edge,
                                     compare_func::less_equal);

    float4 colorSample = colorTextures.sample(textureSampler, in.textureCoordinate, in.diffuseTextureIndex);
    float3 color = colorSample.rgb;
    float3 normal = normalize(in.normal);

    float3 shadowNDC = in.shadowPosition.xyz / in.shadowPosition.w;
    float2 shadowUV  = shadowNDC.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    float bias = 0.0015;
    float currentDepth = shadowNDC.z - bias;

    float3 lightPos   = float3(-0.5, 0.0, 0.0);
    float3 lightColor = float3(0.3);

    float lit = 1.0;
    if (shadowUV.x >= 0.0 && shadowUV.x <= 1.0 &&
        shadowUV.y >= 0.0 && shadowUV.y <= 1.0 &&
        currentDepth <= 1.0)
    {
        lit = shadowMap.sample_compare(shadowSampler, shadowUV, currentDepth);
    }

    float3 ambient = 0.3 * lightColor;

    float3 lightDir = normalize(lightPos - in.worldPosition);
    float  diff     = max(dot(normal, lightDir), 0.0);
    float3 diffuse  = diff * lightColor;

    float3 viewDir    = normalize(viewPos.xyz - in.worldPosition);
    float3 halfwayDir = normalize(lightDir + viewDir);
    float  spec       = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    float3 specular   = spec * lightColor;

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
