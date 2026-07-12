
#include <metal_stdlib>
using namespace metal;

#include <simd/simd.h>

using namespace simd;



struct NVertexData {
	float3 Position;
	float3 Normal;
    float2 TexCoords;
	float3 Tangent;
	float3 Bitangent;
};


struct N_MVP{
    matrix_float4x4 M;
    matrix_float4x4 V;
    matrix_float4x4 P;
};


struct VertexData {
    float4 position;
    float2 textureCoordinate;
};

struct MVP{
    matrix_float4x4 MVP;
};

struct Uniforms
{
    float2 time;
};

struct N_Uniforms{
     float3 lightPos;
     float3 viewPos;
     float3x3 normalMatrix;
};


struct VertexOut {
    float4 position [[position]];
    float3 FragPos ;
    float2 TexCoords;
    float3 TangentLightPos;
    float3 TangentViewPos;
    float3 TangentFragPos;
};

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                              constant NVertexData* vertexData [[buffer(0)]],
                              constant N_Uniforms& uniforms [[buffer(1)]],
                              constant N_MVP & mvp [[buffer(2)]] ) {
        VertexOut vs_out;
        // out.position = mvp.MVP * vertexData[vertexID].position;
        // out.textureCoordinate = vertexData[vertexID].textureCoordinate;

        vs_out.FragPos = float3(mvp.M * float4(vertexData[vertexID].Position, 1.0));   
        vs_out.TexCoords = vertexData[vertexID].TexCoords;
        float3x3 normalMatrix = uniforms.normalMatrix;
        float3 T = normalize(normalMatrix * vertexData[vertexID].Tangent);
        float3 N = normalize(normalMatrix * vertexData[vertexID].Normal);
        T = normalize(T - dot(T, N) * N);
        float3 B = cross(N, T);
        float3x3 TBN = transpose(float3x3(T, B, N));    
        vs_out.TangentLightPos = TBN * uniforms.lightPos;
        vs_out.TangentViewPos  = TBN * uniforms.viewPos;
        vs_out.TangentFragPos  = TBN * vs_out.FragPos;
        vs_out.position = mvp.P * mvp.V * mvp.M * float4(vertexData[vertexID].Position, 1.0);


        return vs_out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> DiffuseMap [[texture(0)]],
                               texture2d<float> NormalMap [[texture(1)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    

    float3 normal = NormalMap.sample(textureSampler, in.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    float3 color = DiffuseMap.sample(textureSampler, in.TexCoords).rgb;
    float3 ambient = 0.1 * color;
    float3 lightDir = normalize(in.TangentLightPos - in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    float3 diffuse = diff * color;
    float3 viewDir = normalize(in.TangentViewPos - in.TangentFragPos);
    // float3 reflectDir = reflect(-lightDir, normal);
    float3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    float3 specular = float3(0.2) * spec;

   
     

    return float4(ambient + diffuse + specular, 1.0);
}


