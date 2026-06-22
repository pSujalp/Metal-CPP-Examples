//
//  Texture.hpp
//  Metal-Tutorial
//

#pragma once
#include <Metal/Metal.hpp>
#include <stb_image.h>

// ── 2-D texture (unchanged) ──────────────────────────────────────────────────
class Texture {
public:
    Texture(const char* filepath, MTL::Device* metalDevice);
    ~Texture();
    MTL::Texture* texture;
    int width, height, channels;
private:
    MTL::Device* device;
};

// ── Cubemap texture ───────────────────────────────────────────────────────────
// Pass 6 separate image paths in Metal's face order:
//   0 = +X (right)
//   1 = -X (left)
//   2 = +Y (top)
//   3 = -Y (bottom)
//   4 = +Z (front)
//   5 = -Z (back)
// All 6 images must be the same square resolution.
class CubeTexture {
public:
    CubeTexture(const char* facePaths[6], MTL::Device* metalDevice);
    ~CubeTexture();
    MTL::Texture* texture;
private:
    MTL::Device* device;
};