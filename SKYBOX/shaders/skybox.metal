//
//  skybox.metal
//  Metal-Tutorial
//
//  Samples a single equirectangular (lat-long) panorama PNG as a skybox.
//  The vertex position IS the cubemap direction vector — no separate UV needed.
//

#include <metal_stdlib>
using namespace metal;


struct MVP {
    float4x4 MVP;
};

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
    texture2d<float>       skyTex   [[texture(0)]])
{
    constexpr sampler s(filter::linear, address::repeat);

    float3 d = normalize(in.direction);

    float u = atan2(d.z, d.x) / (2.0 * M_PI_F) + 0.5;
    float v = asin(clamp(d.y, -1.0f, 1.0f)) / M_PI_F + 0.5;

    return skyTex.sample(s, float2(u, v));
}
