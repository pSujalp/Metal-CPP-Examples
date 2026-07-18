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


float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}




float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * 3.14159265359 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    float3 up = (abs(N.z) < 0.999) ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}


float3 PrefilterEnvMap(texturecube<float> envMap, sampler envSampler, float3 R, float roughness)
{
    
    if (roughness < 0.02) {
        return envMap.sample(envSampler, R).rgb;
    }

    float3 N = R;
    float3 V = R;

    float3 prefilteredColor = float3(0.0);
    float  totalWeight = 0.0;

    const uint SAMPLE_COUNT = 1024; 

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, roughness);
        float3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            prefilteredColor += envMap.sample(envSampler, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    return prefilteredColor / max(totalWeight, 0.0001);
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
                               sampler cubeSampler           [[sampler(0)]],
                               texture2d<float> albedoTex [[texture(0)]],
                               texture2d<float> normalTex [[texture(1)]],
                               texture2d<float> metallicTex [[texture(2)]],
                               texture2d<float> roughnessTex [[texture(3)]],
                               texture2d<float> aoTex [[texture(4)]],
                               texture2d<float> eTex [[texture(5)]],
                               texturecube<float> irradianceMap [[texture(6)]],
                               texturecube<float> CubeMap [[texture(7)]],
                               texture2d<float> BRD_FLUT_Tex [[texture(8)]]
                               ) {

    constexpr sampler texSampler(mip_filter::linear,
                                  mag_filter::linear,
                                  min_filter::linear,
                                  address::repeat);

    constexpr sampler envSampler(mip_filter::linear,
                                  mag_filter::linear,
                                  min_filter::linear,
                                  address::clamp_to_edge);

    constexpr sampler lutSampler(mag_filter::linear,
                                  min_filter::linear,
                                  address::clamp_to_edge);

    float3 albedo    = albedoTex.sample(texSampler, in.TexCoords).rgb;
    float  metallic  = metallicTex.sample(texSampler, in.TexCoords).r;
    float  roughness = max(roughnessTex.sample(texSampler, in.TexCoords).r, 0.045);
    float  ao        = aoTex.sample(texSampler, in.TexCoords).r;
    float3 emissive  = eTex.sample(texSampler, in.TexCoords).rgb;

    float3 Ngeom = normalize(in.Normal);

    float3 dp1 = dfdx(in.WorldPos);
    float3 dp2 = dfdy(in.WorldPos);
    float2 duv1 = dfdx(in.TexCoords);
    float2 duv2 = dfdy(in.TexCoords);

    float3 dp2perp = cross(dp2, Ngeom);
    float3 dp1perp = cross(Ngeom, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    T *= invmax;
    B *= invmax;

    float3x3 TBN = float3x3(T, B, Ngeom);

    float3 tangentNormal = normalTex.sample(texSampler, in.TexCoords).rgb * 2.0 - 1.0;
    float3 N = normalize(TBN * tangentNormal);

    float3 V = normalize(uniforms.cameraPosition - in.WorldPos);
    float3 R = reflect(-V, N);

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
    float3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator   = NDF * G * F;
    float  denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular    = numerator / denominator;

    float3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
    float3 kD = float3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / 3.14159265359 + specular) * radiance * NdotL;

   
    float NdotV = max(dot(N, V), 0.0);
    float3 kSAmbient = fresnelSchlickRoughness(NdotV, F0, roughness);
    float3 kDAmbient = (1.0 - kSAmbient) * (1.0 - metallic);

    float3 irradiance = irradianceMap.sample(envSampler, N).rgb;
    float3 diffuse = irradiance * albedo;

   
    float3 prefilteredColor = PrefilterEnvMap(CubeMap, envSampler, R, roughness);

    float2 brdf = BRD_FLUT_Tex.sample(lutSampler, float2(NdotV, roughness)).rg;
    float3 specular1 = prefilteredColor * (kSAmbient * brdf.x + brdf.y);

    float3 ambient = (kDAmbient * diffuse + specular1) * ao;

    float3 color = ambient + Lo + emissive * 30.0;

    color = color / (color + float3(1.0));
    color = pow(color, float3(1.0 / 2.2));
    return float4(color, 1.0);
}