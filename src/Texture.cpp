




#include <simd/simd.h>
#include "Texture.hpp"




Texture::Texture(const char* filepath, MTL::Device* metalDevice, bool isRGB) {
    device = metalDevice;
    
    stbi_set_flip_vertically_on_load(true);
    
    
    
    
    
    unsigned char* image = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
    assert(image != NULL);
    
    MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();

    
    
    
    
    
    textureDescriptor->setPixelFormat(isRGB ? MTL::PixelFormatRGBA8Unorm_sRGB
                                             : MTL::PixelFormatRGBA8Unorm);

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

Texture::~Texture() {
    texture->release();
}