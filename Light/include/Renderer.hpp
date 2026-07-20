#pragma once 
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include "Shader.h"
#include "VertexData.hpp"
#include "simd/simd.h"
#include "AAPLMathUtilities.h"

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();
        void draw( MTK::View* pView );
    
        void createBuffers();
        void buildShaders();
        void CreateCubeAndLight();
        void createDefaultLibrary();
        void createLightSourceRenderPipeline();

    private:
        MTL::Library* metalDefaultLibrary;
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::RenderPipelineState* _pPSO;
        MTL::RenderPipelineState* metalLightSourceRenderPSO;
        MTL::Buffer* cubeVertexBuffer;
        MTL::Buffer* lightVertexBuffer;
        MTL::Buffer * cubeTransformationBuffer ;
        MTL::Buffer* lightTransformationBuffer;
        MTL::RenderPipelineState* metalRenderPSO;
        MTL::DepthStencilState* depthStencilState;
        float _time = 0.0f;

};