#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include "simd/simd.h"
#include <vector>
#include "VertexData.hpp"
#include "Texture.hpp"
#include <thread>
#include <chrono>


class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();
        void draw( MTK::View* pView );
    
        void createSquare();
        void buildShaders();
        void createDefaultLibrary(MTL::Device* pDevice );

    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::RenderPipelineState* _pPSO;
        Texture* grassTexture;
        Texture* smileyTexture;
        MTL::Buffer* squareVertexBuffer;
        MTL::Buffer* UniformBuffer;
        MTL::Buffer* Uniform1Buffer;
        MTL::Library * metallibrary;
        uint8_t frameCount = 0;

};