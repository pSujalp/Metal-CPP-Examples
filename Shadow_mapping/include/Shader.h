#pragma once

#include <filesystem>
#include <string>
#include <fstream>
#include <sstream>


#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

class Shader{
    private:
        std::string Code;
        std::ifstream ShaderFile;
        std::filesystem::path executableDirectory() const{
#if defined(_WIN32)
            char buffer[MAX_PATH];
            const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
            return std::filesystem::path(buffer, buffer + length).parent_path();
#elif defined(__APPLE__)
            uint32_t size = 0;
            _NSGetExecutablePath(nullptr, &size);
            std::string buffer(size, '\0');
            if (_NSGetExecutablePath(buffer.data(), &size) == 0){
                buffer.resize(std::strlen(buffer.c_str()));
                return std::filesystem::path(buffer).parent_path();
            }
#endif
            return std::filesystem::current_path();
        }
    public:
        Shader(){}

        const char* GetShader(const std::string & filepath){
            ShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

            std::filesystem::path shaderPath(filepath);
            if (shaderPath.is_relative()){
                const std::filesystem::path executablePath = executableDirectory();
                const std::filesystem::path candidatePath = executablePath / shaderPath;

                if (std::filesystem::exists(candidatePath)){
                    shaderPath = candidatePath;
                }
            }

            ShaderFile.open(shaderPath);

            std::stringstream vShaderStream;

            vShaderStream << ShaderFile.rdbuf();
            ShaderFile.close();

            Code = vShaderStream.str();

            return Code.c_str();
        }




};