#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <Metal/Metal.hpp>

#include "stb_truetype.h"




class TextRendering
{
public:
    MTL::Texture* texture = nullptr;

private:
    MTL::Device* device = nullptr;
    size_t multiplier = 4;

    std::vector<unsigned char> fontBuffer;
    stbtt_fontinfo info{};

    std::vector<unsigned char> bitmap;      
    std::vector<unsigned char> rgbaBuffer;  

    float scale = 1.0f;
    int b_w = 512;
    int b_h = 128;
    int l_h = 64;

    std::string word;  

public:
    TextRendering(const std::string &filepath, MTL::Device *device, std::string wordpara = "", size_t multiplier = 4)
        : device(device), multiplier(multiplier)
    {
        FILE *fontFile = fopen(filepath.c_str(), "rb");
        if (!fontFile)
        {
            printf("failed to open font file: %s\n", filepath.c_str());
            return;
        }
        fseek(fontFile, 0, SEEK_END);
        long size = ftell(fontFile);
        fseek(fontFile, 0, SEEK_SET);

        fontBuffer.resize(static_cast<size_t>(size));
        fread(fontBuffer.data(), 1, static_cast<size_t>(size), fontFile);
        fclose(fontFile);

        if (!stbtt_InitFont(&info, fontBuffer.data(), 0))
        {
            printf("failed\n");
        }

        b_w = 512 * static_cast<int>(this->multiplier);
        b_h = 128 * static_cast<int>(this->multiplier);
        l_h = 64 * static_cast<int>(this->multiplier);

        bitmap.assign(static_cast<size_t>(b_w) * b_h, 0);
        rgbaBuffer.resize(static_cast<size_t>(b_w) * b_h * 4);

        scale = stbtt_ScaleForPixelHeight(&info, static_cast<float>(l_h));

        
        
        MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm_sRGB);
        textureDescriptor->setWidth(b_w);
        textureDescriptor->setHeight(b_h);
        textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
        textureDescriptor->setStorageMode(MTL::StorageModeShared);
        texture = device->newTexture(textureDescriptor);
        textureDescriptor->release();

        
        
        
        Draw(wordpara);
    }

    void Draw(const std::string &variableword)
    {
        if (variableword == word) return;  

        std::fill(bitmap.begin(), bitmap.end(), 0);

        const char *text = variableword.c_str();
        const int len = static_cast<int>(variableword.size());

        int x = 0;
        int ascent = 0, descent = 0, lineGap = 0;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
        ascent = static_cast<int>(roundf(ascent * scale));

        for (int i = 0; i < len; ++i)
        {
            int ax, lsb;
            stbtt_GetCodepointHMetrics(&info, text[i], &ax, &lsb);

            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&info, text[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

            int y = ascent + c_y1;
            int byteOffset = x + static_cast<int>(roundf(lsb * scale)) + y * b_w;
            stbtt_MakeCodepointBitmap(&info, bitmap.data() + byteOffset,
                                       c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, text[i]);

            x += static_cast<int>(roundf(ax * scale));

            if (i + 1 < len)
            {
                x += static_cast<int>(roundf(stbtt_GetCodepointKernAdvance(&info, text[i], text[i + 1]) * scale));
            }
        }

        
        
        
        
        for (int row = 0; row < b_h; ++row)
        {
            const unsigned char *srcRow = bitmap.data() + static_cast<size_t>(b_h - 1 - row) * b_w;
            unsigned char *dstRow = rgbaBuffer.data() + static_cast<size_t>(row) * b_w * 4;
            for (int col = 0; col < b_w; ++col)
            {
                unsigned char v = srcRow[col];
                dstRow[col * 4 + 0] = v;
                dstRow[col * 4 + 1] = v;
                dstRow[col * 4 + 2] = v;
                dstRow[col * 4 + 3] = 255;
            }
        }

        MTL::Region region = MTL::Region(0, 0, 0, b_w, b_h, 1);
        NS::UInteger bytesPerRow = 4 * b_w;
        texture->replaceRegion(region, 0, rgbaBuffer.data(), bytesPerRow);

        word = variableword;  
    }

    ~TextRendering()
    {
        if (texture)
        {
            texture->release();
        }
    }
};