#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;
using namespace simd;


struct VertexData {
    float4 position;
};

struct VertexOut{
   float4 position [[position]];
};

struct MVP{
    matrix_float4x4 MVP;
};


vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                              constant VertexData* vertexData [[buffer(0)]],
                              constant MVP & mvp [[buffer(1)]]){
                VertexOut out;
                out.position = mvp.MVP * vertexData[vertexID].position;
                return out;
}


fragment float4 fragmentShader(VertexOut in [[stage_in]]){
    return float4(0.0f,1.0f,0.0f,1.0f);
}