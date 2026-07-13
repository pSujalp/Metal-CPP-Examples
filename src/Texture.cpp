//
//  Texture.cpp
//  Metal-Tutorial
//

#include "Texture.hpp"

Texture::Texture(const char* filepath, MTL::Device* metalDevice) {
    device = metalDevice;
    
    stbi_set_flip_vertically_on_load(true);

    std::string add = executableDirectory().string() ;
    add.push_back('/');
    add.append(filepath);
    
    std::cout<< add<<"\n";
    unsigned char* image = stbi_load(add.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    assert(image != NULL);
    
    MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
    textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    textureDescriptor->setWidth(width);
    textureDescriptor->setHeight(height);

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
