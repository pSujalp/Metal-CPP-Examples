#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include "Shader.h"
#include "VertexData.h"
#include <simd/simd.h>
#include "Time.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();
        void draw( MTK::View* pView );
        void makePipeline();
        void buildBuffers();

    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::RenderPipelineState* _pPSO;
        MTL::DepthStencilState * depthStencilState;

        MTL::Buffer* cubeVertexBuffer;
        MTL::Buffer* transformationBuffer;
        

};