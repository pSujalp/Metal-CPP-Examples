#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include "simd/simd.h"
#include "VertexData.hpp"
#include "Texture.hpp"
#include "AAPLMathUtilities.h"
#include "Time.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>


static const size_t kNumInstances = 1000000;

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();
        void draw( MTK::View* pView );
    
        void createSquare();
        void buildShaders();
        void createDefaultLibrary(MTL::Device* pDevice );
        void CreateCube();


    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::RenderPipelineState* _pPSO;
        Texture* grassTexture;
        MTL::Buffer* squareVertexBuffer;
        MTL::Buffer* UniformBuffer;
        MTL::Buffer* transformationCubeBuffer;
        MTL::Buffer * cubeVertexBuffer;
        MTL::Library * metallibrary;
        MTL::DepthStencilState* depthStencilState;

        static const int kMaxFramesInFlight = 3;

        MTL::Buffer* _pInstanceDataBuffer[kMaxFramesInFlight];
        dispatch_semaphore_t _semaphore;
        
        int _frame;

        std::vector<glm::vec3 >instancePositions;

        

       
};