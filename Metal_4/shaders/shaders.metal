
#include <metal_stdlib>
using namespace metal;


static constant uint VERTEX_BUFFER_BINDING_IDX = 0;


struct VertexIn
{
    float4 position;
    float4 color;
};

struct VertexOut
{
    float4 position [[position]];
    float4 color;
};


vertex float4
vertexShader(uint vertexID [[vertex_id]],
             constant simd::float3* vertexPositions [[ buffer( VERTEX_BUFFER_BINDING_IDX ) ]] )
{
    float4 vertexOutPositions = float4(vertexPositions[vertexID][0],
                                       vertexPositions[vertexID][1],
                                       vertexPositions[vertexID][2],
                                       1.0f);
    return vertexOutPositions;
}

fragment float4 fragmentShader(float4 vertexOutPositions [[stage_in]]) {
    return float4(182.0f/255.0f, 240.0f/255.0f, 228.0f/255.0f, 1.0f);
}