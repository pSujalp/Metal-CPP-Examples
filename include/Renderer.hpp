#pragma once 

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>
#include <Foundation/NSArray.hpp>

#include <ShaderParams.h>
#include "Utilities.h"
#include "Matrix.h"


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
        const size_t bufferSize = arraylenght * sizeof(float);

        MTL::RenderPipelineState* _pRPSO;

    // ------------------------------------------------------------

        

        MTL::ComputePipelineState *m_MatMultiplyFunctionPSO;

        MTL::Buffer *m_device_buffer_A_ptr;

        MTL::Buffer *m_device_buffer_B_ptr;

        MTL::Buffer *m_device_buffer_X_ptr;

        MTL::Buffer *m_device_buffer_params_ptr;

        int m_rows_X = 4;
        int m_cols_X = 4;
        int m_cols_A = 4;
};