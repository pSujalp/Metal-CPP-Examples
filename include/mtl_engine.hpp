//
//  mtl_engine.hpp
//  MetalTutorial
//

#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include <QuartzCore/CAMetalLayer.h>
#include "Shader.h"
#include "VertexData.hpp"
#include "simd/simd.h"

#include <Metal/MTL4CommandBuffer.hpp>
#include <Metal/MTL4CommandQueue.hpp>
#include <Metal/MTL4RenderPipeline.hpp>
#include <Metal/MTL4RenderPass.hpp>


#include <iostream>

class MTLEngine {
public:
    void init();
    void run();
    void cleanup();

private:
    void initDevice();
    void initWindow();
    
    void createTriangle();
    void createCommandQueue();
    void createRenderPipeline();
    
    void encodeRenderCommand(MTL::RenderCommandEncoder* renderEncoder);
    void encodeRenderCommand_M4(MTL4::RenderCommandEncoder* renderCommandEncoder);
    void sendRenderCommand();
    void draw();
    
    MTL::Device* metalDevice;
    GLFWwindow* glfwWindow;
    NSWindow* metalWindow;
    CAMetalLayer* metalLayer;
    CA::MetalDrawable* metalDrawable;
    
    MTL::CommandQueue* metalCommandQueue;
    MTL::CommandBuffer* metalCommandBuffer;
    MTL::RenderPipelineState* metalRenderPSO;
    MTL::Buffer* triangleVertexBuffer;



    MTL::Library* shader_lib;
    MTL4::CommandQueue* metal4CommandQueue;
    MTL4::CommandBuffer* metal4CommandBuffer;
    MTL::RenderPipelineState* metalRenderPSO1;

    Array< MTL::Buffer*, MAX_FRAMES_IN_FLIGHT > vertex_buffers;
};