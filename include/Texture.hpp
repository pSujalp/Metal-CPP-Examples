//
//  Texture.hpp
//  Metal-Tutorial
//

#pragma once
#include <Metal/Metal.hpp>
#include <stb_image.h>

#include <filesystem>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mach-o/dyld.h>



class Texture {
public:
    std::filesystem::path executableDirectory() const
    {

        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::string buffer(size, '\0');
        if (_NSGetExecutablePath(buffer.data(), &size) == 0)
        {
            buffer.resize(std::strlen(buffer.c_str()));
            return std::filesystem::path(buffer).parent_path();
        }

        return std::filesystem::current_path();
    }


    Texture(const char* filepath, MTL::Device* metalDevice);
    ~Texture();
    MTL::Texture* texture;
    int width, height, channels;

private:
    MTL::Device* device;
};
