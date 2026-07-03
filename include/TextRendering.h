#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <cmath>
#include <Metal/Metal.hpp>

#include "stb_image_write.h"
#include "stb_truetype.h"
#include "stb_image.h"



class TextRendering
{
public: MTL::Texture* texture;
private:
    MTL::Texture* Texttexture;
    MTL::Device* device;

    FILE *fontFile;
    std::vector<unsigned char> png;
    long size;
    unsigned char *fontBuffer;
    stbtt_fontinfo info;
    unsigned char *bitmap;
    float scale;
    int b_w = 512;
    int b_h = 128;
    int l_h = 64; 
    std::string word;
public:
    static void write_to_mem(void *context, void *data, int size)
    {
        auto *buf = static_cast<std::vector<unsigned char> *>(context);
        auto *bytes = static_cast<unsigned char *>(data);
        buf->insert(buf->end(), bytes, bytes + size);
    }

    
    TextRendering(const std::string &filepath ,MTL::Device * device)
    {

        fontFile = fopen(filepath.c_str(), "rb");
        fseek(fontFile, 0, SEEK_END);
        size = ftell(fontFile);      
        fseek(fontFile, 0, SEEK_SET);

        fontBuffer = (unsigned char *)malloc(size);

        fread(fontBuffer, size, 1, fontFile);
        fclose(fontFile);

       

        if (!stbtt_InitFont(&info, fontBuffer, 0))
        {
            printf("failed\n");
        }

        b_w = 512;
        b_h = 128;
        l_h = 64; 

       
        bitmap = (unsigned char *)calloc(b_w * b_h, sizeof(unsigned char));

       
        scale = stbtt_ScaleForPixelHeight(&info, l_h);

        this->device = device;
    }

    void Draw(std::string variableword)
    {

        char *word = variableword.data();
        int x = 0;
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
        ascent = roundf(ascent * scale);
        descent = roundf(descent * scale);

        int i;
        for (i = 0; i < strlen(word); ++i)
        {
           
            int ax;
            int lsb;
            stbtt_GetCodepointHMetrics(&info, word[i], &ax, &lsb);
           

           
            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&info, word[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

           
            int y = ascent + c_y1;

           
            int byteOffset = x + roundf(lsb * scale) + (y * b_w);
            stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, word[i]);           
            x += roundf(ax * scale);           
            int kern;
            kern = stbtt_GetCodepointKernAdvance(&info, word[i], word[i + 1]);
            x += roundf(kern * scale);
        }

        stbi_write_png_to_func(write_to_mem, &png, b_w, b_h, 1, bitmap, b_w);
        stbi_set_flip_vertically_on_load(true);

        int width, height, channels;
        unsigned char *image = stbi_load_from_memory(
            png.data(), (int)png.size(),
            &width, &height, &channels,
            STBI_rgb_alpha);

        MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm_sRGB);
        textureDescriptor->setWidth(width);
        textureDescriptor->setHeight(height);
        textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
        textureDescriptor->setStorageMode(MTL::StorageModeShared);

    Texttexture = device->newTexture(textureDescriptor);
    MTL::Region region = MTL::Region(0, 0, 0, width, height, 1);
    NS::UInteger bytesPerRow = 4 * width;
    Texttexture->replaceRegion(region, 0, image, bytesPerRow);
    textureDescriptor->release();
    stbi_image_free(image);

    }

    
    ~TextRendering()
    {
        free(fontBuffer);
        free(bitmap);
    }
};