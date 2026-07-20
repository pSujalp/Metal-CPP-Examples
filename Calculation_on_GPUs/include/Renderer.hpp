#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>
#include <Foundation/NSArray.hpp>

class Renderer
{
    public:
        Renderer( MTL::Device* pDevice );
        ~Renderer();
        void draw( MTK::View* pView );
        void makePipeline();
        void buildBuffers();
        void generateRandomFloatData(MTL::Buffer *buffer);
        void verifyResult();

    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::ComputePipelineState *_pPSO;
        MTL::Buffer *__MBufferA;
        MTL::Buffer *__MBufferB;
        MTL::Buffer *__MBufferResult;
        const size_t arraylenght = 32;
        const size_t bufferSize = arraylenght * sizeof(float); // correct

        MTL::RenderPipelineState* _pRPSO;
        
};