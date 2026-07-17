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

#include <glm/vec3.hpp>                  // glm::vec3
#include <glm/vec4.hpp>                  // glm::vec4
#include <glm/mat4x4.hpp>                // glm::mat4
#include <glm/ext/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp>  // glm::pi
#include <glm/glm.hpp>

#include <glm/gtc/matrix_inverse.hpp>

#include <iostream>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include <stdio.h>

#include "Sphere.h"
#include "SingleMesh.h"

class MTLEngine
{
public:
    void init();
    void run();
    void cleanup();

private:
    void initDevice();
    void initWindow();

    void createSphere();
    void createBuffers();
    void createDefaultLibrary();
    MTL::Library *loadLibrary(MTL::Device *device, const char *path);
    void createCommandQueue();
    void createRenderPipeline();
    void createDepthAndMSAATextures();
    void createRenderPassDescriptor();

    // Upon resizing, update Depth and MSAA Textures.
    void updateRenderPassDescriptor();
    void buildSkyBoxShaders();

    void encodeRenderCommand(MTL::RenderCommandEncoder *renderEncoder);
    void sendRenderCommand();
    void draw();
    void CreateSkyBox();

    static void frameBufferSizeCallback(GLFWwindow *window, int width, int height);
    void resizeFrameBuffer(int width, int height);
    static inline matrix_float4x4 toSimd(const glm::mat4 &m);
    static inline matrix_float3x3 toSimd(const glm::mat3 &m);
    static inline simd::float3 toSimd(const glm::vec3 &v);

    MTL::Device *metalDevice;
    GLFWwindow *glfwWindow;
    NSWindow *metalWindow;
    CAMetalLayer *metalLayer;
    CA::MetalDrawable *metalDrawable;

    MTL::Library *metalDefaultLibrary;

    MTL::CommandQueue *metalCommandQueue;

    MTL::RenderPipelineState *metalRenderPSO;

    MTL::DepthStencilState *depthStencilState;

    MTL::RenderPassDescriptor *renderPassDescriptor;
    MTL::Texture *msaaRenderTargetTexture = nullptr;
    MTL::Texture *depthTexture;
    int sampleCount = 4;

    Texture *grassTexture;

    bool show_demo_window = true;

    float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

    Mesh *sphere;

    static const int kMaxDrawsPerFrame = 10;
    MTL::CommandBuffer *metalCommandBuffer;

    MTL::Buffer *SphereVertexBuffer[kMaxDrawsPerFrame];
    MTL::Buffer *SphereIndexedBuffer[kMaxDrawsPerFrame];
    MTL::Buffer *SphereUVBuffer[kMaxDrawsPerFrame];
    MTL::Buffer *SphereNormalBuffer[kMaxDrawsPerFrame];
    MTL::Buffer *uniformsBuffer[kMaxDrawsPerFrame];
    MTL::Buffer *transformationBuffer[kMaxDrawsPerFrame];
    uint8_t index = 0;

    MTL::Buffer *SkyBoxVertexBuffer[kMaxDrawsPerFrame];
    MTL::Buffer *MVPSkyBoxBuffer[kMaxDrawsPerFrame];
    CubeTexture *skyboxTexture;
    MTL::Buffer *SamplerBuffer;
    MTL::SamplerState* samplerState;

    
    MTL::RenderPipelineState *_SkyboxPSO;
    MTL::DepthStencilState *SkyBoxDepthStencilState;
    MTL::Library *metalSkyBoxlibrary;

    // IMGUI

    int lightintensity = 0;
    float Location[3] = {0.0f, 0.0f, 0.0f};
    float LightPosition[3] = {3.0f, 0.0f, 5.0f};
    float metallic = 0.5;
    float roughness = 0;
    float ao = 1.0f;

    float myColor[4] = {0.5f, 0.0f, 0.0f, 1.0f};

    Texture * Albedo;
    Texture * AO;
    Texture * Metallic;
    Texture * Normal;
    Texture * Roughness;
    Texture * Emissive; 
};