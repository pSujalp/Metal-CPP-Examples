#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include "simd/simd.h"

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();
        void draw( MTK::View* pView );
    
        void buildBuffers();
        void buildShaders();

    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::RenderPipelineState* _pPSO;
        MTL::Buffer* _pVertexPositionsBuffer;
        MTL::Buffer* _pVertexColorsBuffer;
};