#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include "simd/simd.h"
#include "VertexData.hpp"
#include "Texture.hpp"

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
        MTL::Buffer* squareVertexBuffer;
        MTL::Buffer* UniformBuffer;
        MTL::Buffer* Uniform1Buffer;
        MTL::Library * metallibrary;
};