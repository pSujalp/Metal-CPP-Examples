//
//  skybox.metal
//  Metal-Tutorial
//
//  Samples a single equirectangular (lat-long) panorama PNG as a skybox.
//  The vertex position IS the cubemap direction vector — no separate UV needed.
//

#include <metal_stdlib>
using namespace metal;

// Must match the MVP struct in VertexData.hpp
struct MVP {
    float4x4 MVP;
};

struct SkyboxVOut {
    float4 position [[position]];
    float3 direction;        // world-space direction for equirect lookup
};

// ─── Vertex ──────────────────────────────────────────────────────────────────
vertex SkyboxVOut skyboxVertex(
    uint                   vid      [[vertex_id]],
    constant float4*       verts    [[buffer(0)]],
    constant MVP&          mvp      [[buffer(2)]])
{
    float4 pos = verts[vid];

    SkyboxVOut out;
    // Store the raw xyz as the sample direction BEFORE projection
    out.direction = pos.xyz;

    // Project — force depth to 1.0 (far plane) by outputting w,w instead of z,w
    float4 clip = mvp.MVP * pos;
    out.position = clip.xyww;   // z = w → NDC depth = 1.0 after divide

    return out;
}

// ─── Fragment ─────────────────────────────────────────────────────────────────
// Converts a 3-D direction vector to (u, v) on an equirectangular map:
//   u = atan2(z, x) / (2π) + 0.5      → [0, 1] longitude
//   v = asin(y / |dir|) / π + 0.5     → [0, 1] latitude
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
