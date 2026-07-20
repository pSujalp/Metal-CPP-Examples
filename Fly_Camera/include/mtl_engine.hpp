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
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/QuartzCore.hpp>
#include <simd/simd.h>


#include "VertexData.hpp"
#include "Texture.hpp"
#include <stb_image.h>
#include "AAPLMathUtilities.h"
#include "Shader.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <filesystem>

#include "Camera.h"

class MTLEngine {
public:
    void init();
    void run();
    void cleanup();

    Camera * camera;

private:
    void initDevice();
    void initWindow();
    
    void createCube();
    void createBuffers();
    void createDefaultLibrary();
    MTL::Library* loadLibrary(MTL::Device* device, const char* path);
    void createCommandQueue();
    void createRenderPipeline();
    void createDepthAndMSAATextures();
    void createRenderPassDescriptor();

    // Upon resizing, update Depth and MSAA Textures.
    void updateRenderPassDescriptor();
    
    void encodeRenderCommand(MTL::RenderCommandEncoder* renderEncoder);
    void sendRenderCommand();
    void draw();
    void processInput(GLFWwindow *window);
    
    static  void frameBufferSizeCallback(GLFWwindow *window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    void resizeFrameBuffer(int width, int height);
    
    MTL::Device* metalDevice;
    GLFWwindow* glfwWindow;
    NSWindow* metalWindow;
    CAMetalLayer* metalLayer;
    CA::MetalDrawable* metalDrawable;
    
    MTL::Library* metalDefaultLibrary;
    MTL::CommandQueue* metalCommandQueue;
    MTL::CommandBuffer* metalCommandBuffer;
    MTL::RenderPipelineState* metalRenderPSO;
    MTL::Buffer* cubeVertexBuffer;
    MTL::Buffer* transformationBuffer;
    MTL::DepthStencilState* depthStencilState;
    MTL::RenderPassDescriptor* renderPassDescriptor;
    MTL::Texture* msaaRenderTargetTexture = nullptr;
    MTL::Texture* depthTexture;
    int sampleCount = 4;
    
    Texture* grassTexture;

    

    float lastX;
    float lastY;
    bool firstMouse = true;
    bool rightMouseButtonPressed = false;
    float deltaTime = 0.0f;	// time between current frame and last frame
    float lastFrame = 0.0f;
};