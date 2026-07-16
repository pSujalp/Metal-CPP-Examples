
#include <metal_stdlib>
using namespace metal;

#include <simd/simd.h>

using namespace simd;


#include "VertexData.hpp"




struct SkyboxVOut {
    float4 position [[position]];
    float3 direction;  
};

vertex SkyboxVOut skyboxVertex(
    uint                   vid      [[vertex_id]],
    constant float4*       verts    [[buffer(0)]],
    constant MVP&          mvp      [[buffer(2)]])
{
    float4 pos = verts[vid];
    SkyboxVOut out;
    out.direction = pos.xyz;
    float4 clip = mvp.MVP * pos;
    out.position = clip.xyww;  
    return out;
}

fragment float4 skyboxFragment(
    SkyboxVOut             in       [[stage_in]],
    texturecube<half>       skyTex   [[texture(0)]],
    sampler cubeSampler           [[sampler(0)]]
    )
{
    float3 texCoords = float3(in.direction.x, in.direction.y, -in.direction.z);
    
    return float4(skyTex.sample(cubeSampler, texCoords));
    
}