
#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;
using namespace simd;

struct NVertexData{
    float3 position;
    float3 TexCoords;
    float3 normal;
    float3 tangent;
    float3 bitangent;

};

struct N_MVP{
    matrix_float4x4 MVP;
};


struct VertexOut {
    float4 FragPos [[position]];
    float3 TexCoords;
    float3 TangentLightPos;
    float3 TangentViewPos;
    float3 TangentFragPos;
} ;


vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                              constant NVertexData* vertexData [[buffer(0)]],
                              constant N_MVP & mvp [[buffer(1)]]) {
    
    
        VertexOut out;
        out.position = mvp.MVP * vertexData[vertexID].position;
        return out;
}


