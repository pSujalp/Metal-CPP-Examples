//
//  cube.metal
//  MetalTutorial
//

#include <metal_stdlib>
using namespace metal;

#include <simd/simd.h>

using namespace simd;
#include "VertexData.hpp"



float DistributionGGX(float3 N, float3 H, float roughness){
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}


float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


struct VertexOut {
    float4 position [[position]];
    float2 TexCoords;
    float3 WorldPos;
    float3 Normal;

};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
             constant VertexDataPosition* vertexData[[buffer(0)]],
             constant VertexDataUV* vertexDataUV [[buffer(1)]],
             constant VertexDataNormal* vertexDataNormal [[buffer(2)]],
             constant TransformationData* transformationData [[buffer(3)]])
{
    VertexOut out;
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * float4(vertexData[vertexID].position,1.0f);
    out.TexCoords = vertexDataUV[vertexID].textureCoordinate;
    out.WorldPos = float3(transformationData->modelMatrix * float4(vertexData[vertexID].position,1.0f));
    out.Normal = transformationData->normalMatrix * vertexDataNormal[vertexID].normal;
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               constant Uniforms& uniforms [[buffer(0)]],
                               texturecube<float> irradianceMap [[texture(6)]],
                               sampler cubeSampler           [[sampler(0)]],
                               texture2d<float> albedoTex [[texture(0)]],
                               texture2d<float> normalTex [[texture(1)]],
                               texture2d<float> metallicTex [[texture(2)]],
                               texture2d<float> roughnessTex [[texture(3)]],
                               texture2d<float> aoTex [[texture(4)]]
                               ) {

   
    constexpr sampler texSampler(mip_filter::linear,
                                  mag_filter::linear,
                                  min_filter::linear,
                                  address::repeat);

    float3 albedo    = albedoTex.sample(texSampler, in.TexCoords).rgb;
    float  metallic  = metallicTex.sample(texSampler, in.TexCoords).r;
    float  roughness = roughnessTex.sample(texSampler, in.TexCoords).r;
    float  ao        = aoTex.sample(texSampler, in.TexCoords).r;

    float3 tangentNormal = normalTex.sample(texSampler, in.TexCoords).xyz * 2.0 - 1.0;

    float3 Q1  = dfdx(in.WorldPos);
    float3 Q2  = dfdy(in.WorldPos);
    float2 st1 = dfdx(in.TexCoords);
    float2 st2 = dfdy(in.TexCoords);

    float3 geomNormal = normalize(in.Normal);
    float3 T  = normalize(Q1*st2.y - Q2*st1.y);
    float3 B  = -normalize(cross(geomNormal, T));
    float3x3 TBN = float3x3(T, B, geomNormal);

    float3 N = normalize(TBN * tangentNormal);
    float3 V = normalize(uniforms.cameraPosition - in.WorldPos);

    float3 F0 = float3(0.04);
    F0 = mix(F0, albedo, metallic);
    float3 Lo = float3(0.0);

    float3 L = normalize(uniforms.lightPosition - in.WorldPos);
    float3 H = normalize(V + L);
    float distance = length(uniforms.lightPosition - in.WorldPos);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = uniforms.lightColor * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    float3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    float3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
    float3 kD = float3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / 3.14159265359 + specular) * radiance * NdotL;

    float3 irradiance = irradianceMap.sample(cubeSampler, N).rgb;
    float3 diffuse = irradiance * albedo;
    float3 ambient = (kD * diffuse) * ao;

    float3 color = ambient + Lo;
    color = color / (color + float3(1.0));
    color = pow(color, float3(1.0/2.2));
    return float4(color, 1.0);
}