//
//  Texture.hpp
//  Metal-Tutorial
//

#pragma once
#include <Metal/Metal.hpp>
#include <stb_image.h>


class Texture {
public:
    Texture(const char* filepath, MTL::Device* metalDevice,bool isRGB = true);
    ~Texture();
    MTL::Texture* texture;
    int width, height, channels;

private:
    MTL::Device* device;
};
