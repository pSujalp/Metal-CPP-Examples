//
//  skybox.metal
//  Metal-Tutorial
//
//  Samples a single equirectangular (lat-long) panorama PNG as a skybox.
//  The vertex position IS the cubemap direction vector — no separate UV needed.
//

#include <metal_stdlib>
using namespace metal;

struct SkyboxVertexData {
    float3 position;
};

struct MVP {
    float4x4 MVP;
};

struct SkyboxVOut {
    float4 position [[position]];
    float3 direction;  
};

vertex SkyboxVOut skyboxVertex(
    uint                   vid      [[vertex_id]],
    constant SkyboxVertexData* verts    [[buffer(0)]],
    constant MVP&          mvp      [[buffer(2)]])
{
    SkyboxVOut out;
    

    out.direction = verts[vid].position;
    float4 clip = mvp.MVP * float4(verts[vid].position, 1.0);
    out.position = clip.xyww;  
    return out;
}

fragment float4 skyboxFragment(
    SkyboxVOut             in       [[stage_in]],
    texturecube<half>      skyTex   [[texture(0)]],
    sampler cubeSampler           [[sampler(0)]])
{
    float3 texCoords = float3(in.direction.x, in.direction.y, -in.direction.z);

    return float4(skyTex.sample(cubeSampler, texCoords));

}
