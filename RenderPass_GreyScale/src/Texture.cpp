//
//  Texture.cpp
//  Metal-Tutorial
//

#include <simd/simd.h>
#include "Texture.hpp"

Texture::Texture(const char *filepath, MTL::Device *metalDevice)
{
    device = metalDevice;

    stbi_set_flip_vertically_on_load(true);
    unsigned char *image = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
    assert(image != NULL);

    MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
    textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm_sRGB);
    textureDescriptor->setWidth(width);
    textureDescriptor->setHeight(height);
    textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
    textureDescriptor->setStorageMode(MTL::StorageModeShared);

    texture = device->newTexture(textureDescriptor);
    MTL::Region region = MTL::Region(0, 0, 0, width, height, 1);
    NS::UInteger bytesPerRow = 4 * width;
    texture->replaceRegion(region, 0, image, bytesPerRow);
    textureDescriptor->release();
    stbi_image_free(image);
}

Texture::~Texture()
{
    texture->release();
}

Texture::Texture(MTL::Device *metalDevice)
{
    device = metalDevice;
    width = 512;
    height = 512;
    channels = 4;

    MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
    textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm_sRGB);
    textureDescriptor->setWidth(width);
    textureDescriptor->setHeight(height);
    textureDescriptor->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget);
    textureDescriptor->setStorageMode(MTL::StorageModePrivate);

    std::vector<uint8_t> pixels(width * height * channels, 0);
    for (int i = 0; i < width * height; i++)
    {
        pixels[i * 4 + 0] = 255; // R
        pixels[i * 4 + 1] = 0;   // G
        pixels[i * 4 + 2] = 0;   // B
        pixels[i * 4 + 3] = 255; // A
    }

    MTL::Region region = MTL::Region::Make2D(0, 0, width, height);
    texture->replaceRegion(region, 0, pixels.data(), width * channels);

    texture = device->newTexture(textureDescriptor);
    textureDescriptor->release();
}