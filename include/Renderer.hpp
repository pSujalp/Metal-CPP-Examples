#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include "simd/simd.h"
#include "mesh.hpp"
#include "VertexData.hpp"

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();
        void draw( MTK::View* pView );
    
        void buildBuffers();
        void buildShaders();
        void createRenderPipeline();
        void createLightSourceRenderPipeline();
        void loadMeshes();

    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::RenderPipelineState* metalRenderPSO;
        MTL::Buffer* _pVertexPositionsBuffer;
        MTL::Buffer* _pVertexColorsBuffer;
        MTL::Buffer * lightVertexBuffer ;
        Mesh* mesh;
        MTL::DepthStencilState* depthStencilState;
        MTL::RenderPipelineState* metalLightSourceRenderPSO;
};