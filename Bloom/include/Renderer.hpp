#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include "simd/simd.h"
#include "VertexData.hpp"
#include "Texture.hpp"
#include "AAPLMathUtilities.h"
#include "Time.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>




class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();
        void draw( MTK::View* pView );
    
        void createSquare();
        void buildShaders();
        void createDefaultLibrary(MTL::Device* pDevice );
        void createLightSourceRenderPipeline();
        void CreateCube();


    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::RenderPipelineState* _pPSO;
        MTL::RenderPipelineState* metalLightSourceRenderPSO;
        Texture* D_Texture;
        Texture* N_Texture;
        MTL::Buffer* squareVertexBuffer;
        MTL::Buffer* UniformBuffer;
        MTL::Buffer* transformationBuffer;
        MTL::Buffer* transformationCubeBuffer;
        MTL::Buffer * lightVertexBuffer;
        MTL::Buffer * cubeVertexBuffer;
        MTL::Library * metallibrary;
        MTL::DepthStencilState* depthStencilState;
        MTL::Buffer * planeVertexBuffer;
        MTL::Texture * _renderTexture;
        MTL::Texture *_MaskTexture;
        MTL::Buffer *lightTransformationBuffer;

        
        MTL::RenderPassDescriptor* _renderToTextureRenderPassDescriptor;
        float _aspectRatio;
        MTL::Texture * _offscreenDepthTexture;
        MTL::RenderPipelineState* _renderToTexturePipelineState;
        

       
};