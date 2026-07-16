#include "mtl_engine.hpp"

void MTLEngine::init()
{
    initDevice();
    initWindow();


     

    createSphere();
    createBuffers();
    createDefaultLibrary();

    buildSkyBoxShaders();
    CreateSkyBox();
    createCommandQueue();
    createRenderPipeline();
    createDepthAndMSAATextures();
    createRenderPassDescriptor();
    
}

void MTLEngine::run()
{
    while (!glfwWindowShouldClose(glfwWindow))
    {
        @autoreleasepool
        {
            metalDrawable = (__bridge CA::MetalDrawable *)[metalLayer nextDrawable];
            draw();
        }
        glfwPollEvents();
    }
}

void MTLEngine::cleanup()
{
    glfwTerminate();
    // transformationBuffer->release();
    msaaRenderTargetTexture->release();
    depthTexture->release();
    renderPassDescriptor->release();
    metalDevice->release();
    delete grassTexture;

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void MTLEngine::initDevice()
{
    metalDevice = MTL::CreateSystemDefaultDevice();
}

void MTLEngine::frameBufferSizeCallback(GLFWwindow *window, int width, int height)
{
    MTLEngine *engine = (MTLEngine *)glfwGetWindowUserPointer(window);
    engine->resizeFrameBuffer(width, height);
}
void MTLEngine::resizeFrameBuffer(int width, int height)
{
    metalLayer.drawableSize = CGSizeMake(width, height);
    // Deallocate the textures if they have been created
    if (msaaRenderTargetTexture)
    {
        msaaRenderTargetTexture->release();
        msaaRenderTargetTexture = nullptr;
    }
    if (depthTexture)
    {
        depthTexture->release();
        depthTexture = nullptr;
    }
    createDepthAndMSAATextures();
    metalDrawable = (__bridge CA::MetalDrawable *)[metalLayer nextDrawable];
    updateRenderPassDescriptor();
}

MTL::Library *MTLEngine::loadLibrary(MTL::Device *device, const char *path)
{
    NS::Error *error = nullptr;

    NS::String *nsPath = NS::String::string(path, NS::StringEncoding::UTF8StringEncoding);
    NS::URL *url = NS::URL::fileURLWithPath(nsPath);

    MTL::Library *library = device->newLibrary(url, &error);

    if (!library)
    {
        printf("Failed to load library at %s: %s\n",
               path,
               error ? error->localizedDescription()->utf8String() : "unknown error");
        assert(false);
    }

    return library;
}

void MTLEngine::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow = glfwCreateWindow(800, 600, "Metal Engine", NULL, NULL);
    if (!glfwWindow)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    glfwSetWindowMonitor(glfwWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow, frameBufferSizeCallback);
    int width, height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);

    metalWindow = glfwGetCocoaWindow(glfwWindow);
    metalLayer = [CAMetalLayer layer];
    metalLayer.device = (__bridge id<MTLDevice>)metalDevice;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.drawableSize = CGSizeMake(width, height);
    metalWindow.contentView.layer = metalLayer;
    metalWindow.contentView.wantsLayer = YES;

    metalDrawable = (__bridge CA::MetalDrawable *)[metalLayer nextDrawable];

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;

    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplMetal_Init((__bridge id<MTLDevice>)(metalDevice));
}

void MTLEngine::createSphere()
{

    sphere = new Sphere(4, 30, 30);

    std::vector<VertexDataPosition> vertexDataposition;

    for (size_t i = 0; i < sphere->positions.size(); i++)
    {
        glm::vec3 t = sphere->positions[i];
        VertexDataPosition vdp;

        vdp.position = *reinterpret_cast<float3 *>(&t);
        vertexDataposition.emplace_back(vdp);
    }

    std::vector<VertexDataUV> vertexDataUV;

    for (size_t i = 0; i < sphere->uv.size(); i++)
    {
        glm::vec2 t = sphere->uv[i];
        VertexDataUV vduv;
        vduv.textureCoordinate = *reinterpret_cast<float2 *>(&t);
        vertexDataUV.emplace_back(vduv);
    }

    std::vector<VertexDataNormal> vertexDataNormal;

    for (size_t i = 0; i < sphere->normals.size(); i++)
    {
        glm::vec3 t = sphere->normals[i];
        VertexDataNormal vdn;
        vdn.normal = *reinterpret_cast<float3 *>(&t);
        vertexDataNormal.emplace_back(vdn);
    }

    // grassTexture = new Texture("assets/mc_grass.jpeg", metalDevice);

    for (size_t i = 0; i < kMaxDrawsPerFrame; i++)
    {
        SphereVertexBuffer[i] = metalDevice->newBuffer(vertexDataposition.data(), vertexDataposition.size() * sizeof(VertexDataPosition), MTL::ResourceStorageModeShared);
        SphereIndexedBuffer[i] = metalDevice->newBuffer(sphere->indices.data(),
                                                        sphere->indices.size() * sizeof(unsigned int),
                                                        MTL::ResourceStorageModeShared);
        ;

        SphereUVBuffer[i] = metalDevice->newBuffer(vertexDataUV.data(),
                                                   vertexDataUV.size() * sizeof(VertexDataUV),
                                                   MTL::ResourceStorageModeShared);
        SphereNormalBuffer[i] = metalDevice->newBuffer(vertexDataNormal.data(),
                                                       vertexDataNormal.size() * sizeof(VertexDataNormal),
                                                       MTL::ResourceStorageModeShared);

        uniformsBuffer[i] = metalDevice->newBuffer(sizeof(Uniforms), MTL::ResourceStorageModeShared);
        ;
        transformationBuffer[i] = metalDevice->newBuffer(sizeof(TransformationData), MTL::ResourceStorageModeShared);


    }
}

void MTLEngine::createBuffers()
{
    
}

void MTLEngine::createDefaultLibrary()
{

    Shader sh;
    std::filesystem::path exeDir = sh.executableDirectory();

    std::string dirStr = exeDir.string();

    dirStr.append("/default.metallib");

    const char *dirCStr = dirStr.c_str();
    metalDefaultLibrary = loadLibrary(metalDevice, dirCStr);

    if (!metalDefaultLibrary)
    {
        std::cerr << "Failed to load default library.";
        std::exit(-1);
    }

    dirStr= "";
    exeDir = sh.executableDirectory();
    dirStr = exeDir.string();
    dirStr.append("/skybox.metallib");
    dirCStr = dirStr.c_str();
    std::cout<< dirCStr;
    metalSkyBoxlibrary = loadLibrary(metalDevice, dirCStr);

    if (!metalSkyBoxlibrary)
    {
        std::cerr << "Failed to load default library.";
        std::exit(-1);
    }

}

void MTLEngine::createCommandQueue()
{
    metalCommandQueue = metalDevice->newCommandQueue();
}

void MTLEngine::createRenderPipeline()
{
    MTL::Function *vertexShader = metalDefaultLibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function *fragmentShader = metalDefaultLibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor *renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer.pixelFormat;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->setSampleCount(sampleCount);
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    NS::Error *error;
    metalRenderPSO = metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);

    if (metalRenderPSO == nil)
    {
        std::cout << "Error creating render pipeline state: " << error << std::endl;
        std::exit(0);
    }

    MTL::DepthStencilDescriptor *depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = metalDevice->newDepthStencilState(depthStencilDescriptor);




    renderPipelineDescriptor->release();
    vertexShader->release();
    fragmentShader->release();
}
void MTLEngine::buildSkyBoxShaders()
{
    NS::Error* pError = nullptr;

    MTL::Function* vert = metalSkyBoxlibrary->newFunction(
        NS::String::string("skyboxVertex", NS::ASCIIStringEncoding));
    assert(vert && "ERROR: 'skyboxVertex' not found");

    MTL::Function* frag = metalSkyBoxlibrary->newFunction(
        NS::String::string("skyboxFragment", NS::ASCIIStringEncoding));
    assert(frag && "ERROR: 'skyboxFragment' not found");

    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(vert);
    pDesc->setFragmentFunction(frag);

    pDesc->setSampleCount(sampleCount);

   
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer.pixelFormat;
    pDesc->colorAttachments()->object(0)->setPixelFormat(pixelFormat);

    pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
    depthDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthDesc->setDepthWriteEnabled(false);
    SkyBoxDepthStencilState = metalDevice->newDepthStencilState(depthDesc);
    depthDesc->release();

    _SkyboxPSO = metalDevice->newRenderPipelineState(pDesc, &pError);
    if (!_SkyboxPSO) {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    frag->release();
    vert->release();
    pDesc->release();
}

void MTLEngine::createDepthAndMSAATextures()
{
    MTL::TextureDescriptor *msaaTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    msaaTextureDescriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    msaaTextureDescriptor->setWidth(metalLayer.drawableSize.width);
    msaaTextureDescriptor->setHeight(metalLayer.drawableSize.height);
    msaaTextureDescriptor->setSampleCount(sampleCount);
    msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);

    msaaRenderTargetTexture = metalDevice->newTexture(msaaTextureDescriptor);

    MTL::TextureDescriptor *depthTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    depthTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    depthTextureDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthTextureDescriptor->setWidth(metalLayer.drawableSize.width);
    depthTextureDescriptor->setHeight(metalLayer.drawableSize.height);
    depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    depthTextureDescriptor->setSampleCount(sampleCount);

    depthTexture = metalDevice->newTexture(depthTextureDescriptor);

    msaaTextureDescriptor->release();
    depthTextureDescriptor->release();
}

void MTLEngine::createRenderPassDescriptor()
{
    renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

    MTL::RenderPassColorAttachmentDescriptor *colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
    MTL::RenderPassDepthAttachmentDescriptor *depthAttachment = renderPassDescriptor->depthAttachment();

    colorAttachment->setTexture(msaaRenderTargetTexture);
    colorAttachment->setResolveTexture(metalDrawable->texture());
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setClearColor(MTL::ClearColor(0 / 255.0f, 0 / 255.0f, 0.0f / 255.0f, 1.0));
    colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);

    depthAttachment->setTexture(depthTexture);
    depthAttachment->setLoadAction(MTL::LoadActionClear);
    depthAttachment->setStoreAction(MTL::StoreActionDontCare);
    depthAttachment->setClearDepth(1.0);
}

void MTLEngine::updateRenderPassDescriptor()
{
    renderPassDescriptor->colorAttachments()->object(0)->setTexture(msaaRenderTargetTexture);
    renderPassDescriptor->colorAttachments()->object(0)->setResolveTexture(metalDrawable->texture());
    renderPassDescriptor->depthAttachment()->setTexture(depthTexture);
}

void MTLEngine::draw()
{
    sendRenderCommand();
}

void MTLEngine::sendRenderCommand()
{

    

    metalCommandBuffer = metalCommandQueue->commandBuffer();

    updateRenderPassDescriptor();
    MTL::RenderCommandEncoder *renderCommandEncoder = metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);
    encodeRenderCommand(renderCommandEncoder);
    renderCommandEncoder->endEncoding();

    ImGui_ImplMetal_NewFrame((__bridge MTLRenderPassDescriptor *)renderPassDescriptor);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove;
    
    
    ImGui::Begin("PBR Values", nullptr, window_flags);


    ImGui::SliderInt("Adjust Light Intensity", &lightintensity, 0, 2000);
    // ImGui::SliderFloat3("ALBEDO_COLOR", AlbedoColor, 0.0f , 1.0f);
    ImGui::SliderFloat3("LIGHT Position", LightPosition, 0.0f , 10);
    ImGui::SliderFloat("METALLIC", &metallic, 0, 1.0f);
    ImGui::SliderFloat("ROUGNESS", &roughness, 0, 1.0f);
    ImGui::SliderFloat("AO", &ao, 0, 1.0f);

    ImGui::Spacing();

    ImGui::Spacing();


    ImGuiColorEditFlags wheelFlags = ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs;
    ImGui::ColorPicker4("##WheelPicker", myColor, wheelFlags);
    


    ImGui::End();
    

    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), (__bridge id<MTLCommandBuffer>)metalCommandBuffer,
                                   (__bridge id<MTLRenderCommandEncoder>)renderCommandEncoder);

    metalCommandBuffer->presentDrawable(metalDrawable);
    metalCommandBuffer->commit();
    metalCommandBuffer->waitUntilCompleted();
}

void MTLEngine::encodeRenderCommand(MTL::RenderCommandEncoder *renderCommandEncoder)
{
    index = (index + 1) % kMaxDrawsPerFrame;

    // Moves the sphere 10 units down the negative Z-axis
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));

    float angleInDegrees = static_cast<float>(glfwGetTime()) / 2.0f * 45.0f;
    float angleInRadians = glm::radians(angleInDegrees);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angleInRadians, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 modelMatrix = translationMatrix * rotationMatrix;

    float time = static_cast<float>(glfwGetTime());
    float oscillation = sin(time);             // oscillates between -1 and 1
    float zPosition = 1.5f + 1.5f * oscillation; // maps oscillation to range [0, 3]

    glm::vec3 cameraPos    = glm::vec3(0.0f, 0.0f, 1.0f);  // Camera Position in World Space
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // P + F, where F = (0,0,-1)
    glm::vec3 upVector     = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 viewMatrix = glm::lookAtRH(cameraPos, cameraTarget, upVector);

    float aspectRatio = static_cast<float>(metalLayer.frame.size.width / metalLayer.frame.size.height);
    float fov = glm::radians(90.0f);
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    // RH + zero-to-one depth range, matching Metal's clip space (not OpenGL's -1..1)
    glm::mat4 perspectiveMatrix = glm::perspectiveRH_ZO(fov, aspectRatio, nearZ, farZ);

    glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));

    // Convert to simd only here, at the boundary with the GPU-facing struct
    TransformationData transformationData = {
        toSimd(modelMatrix),
        toSimd(viewMatrix),
        toSimd(perspectiveMatrix),
        toSimd(normalMatrix)
    };
    memcpy(transformationBuffer[index]->contents(), &transformationData, sizeof(transformationData));

    glm::mat4 inverseView = glm::inverse(viewMatrix);
    glm::vec3 cameraPosition = glm::vec3(inverseView[3]);

    Uniforms uniforms;
    uniforms.cameraPosition = toSimd(cameraPosition);
    uniforms.lightPosition = simd::float3{LightPosition[0],LightPosition[1],LightPosition[2]};
    uniforms.lightColor = simd::float3{1.0f * lightintensity, 1.0f * lightintensity, 1.0f * lightintensity};

    uniforms.albedo = simd::float3{myColor[0], myColor[1], myColor[2]};
    uniforms.metallic = metallic;
    uniforms.roughness = roughness;
    uniforms.ao = ao;

    memcpy(uniformsBuffer[index]->contents(), &uniforms, sizeof(Uniforms));

    // --- Skybox pass (drawn first, no depth write, matching Renderer.cpp) ---
    static float skyboxDeg = 0.0f;
    skyboxDeg += 1.0f;

    glm::mat4 skyboxModel = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
    skyboxModel = glm::rotate(skyboxModel, glm::radians(skyboxDeg), glm::vec3(0.0f, 1.0f, 0.0f));

    
    glm::mat4 skyboxMVP_GLM =
    perspectiveMatrix *
    glm::mat4(glm::mat3(viewMatrix));

    MVP mvpSkybox;
    mvpSkybox.MVP = matrix_float4x4({
        simd::float4{ skyboxMVP_GLM[0][0], skyboxMVP_GLM[0][1], skyboxMVP_GLM[0][2], skyboxMVP_GLM[0][3] },
        simd::float4{ skyboxMVP_GLM[1][0], skyboxMVP_GLM[1][1], skyboxMVP_GLM[1][2], skyboxMVP_GLM[1][3] },
        simd::float4{ skyboxMVP_GLM[2][0], skyboxMVP_GLM[2][1], skyboxMVP_GLM[2][2], skyboxMVP_GLM[2][3] },
        simd::float4{ skyboxMVP_GLM[3][0], skyboxMVP_GLM[3][1], skyboxMVP_GLM[3][2], skyboxMVP_GLM[3][3] },
    });
    memcpy(MVPSkyBoxBuffer[index]->contents(), &mvpSkybox, sizeof(MVP));

    renderCommandEncoder->setRenderPipelineState(_SkyboxPSO);
renderCommandEncoder->setDepthStencilState(SkyBoxDepthStencilState);
renderCommandEncoder->setCullMode(MTL::CullModeNone);
    renderCommandEncoder->setVertexBuffer(SkyBoxVertexBuffer[index], 0, 0);
    renderCommandEncoder->setVertexBuffer(MVPSkyBoxBuffer[index],    0, 2);
    renderCommandEncoder->setFragmentTexture(skyboxTexture->texture, 0);
    renderCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle,
                         NS::UInteger(0), NS::UInteger(36));

    
    renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    renderCommandEncoder->setCullMode(MTL::CullModeBack);

    renderCommandEncoder->setRenderPipelineState(metalRenderPSO);
    renderCommandEncoder->setDepthStencilState(depthStencilState);

    renderCommandEncoder->setVertexBuffer(SphereVertexBuffer[index], 0, 0);
    renderCommandEncoder->setVertexBuffer(SphereUVBuffer[index], 0, 1);
    renderCommandEncoder->setVertexBuffer(SphereNormalBuffer[index], 0, 2);
    renderCommandEncoder->setVertexBuffer(transformationBuffer[index], 0, 3);
    renderCommandEncoder->setFragmentBuffer(uniformsBuffer[index], 0, 0);
    renderCommandEncoder->setFragmentTexture(skyboxTexture->texture, 0);
    renderCommandEncoder->setFragmentSamplerState(samplerState, 0);
    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;

    renderCommandEncoder->drawIndexedPrimitives(typeTriangle,
                                                sphere->indexCount,
                                                MTL::IndexTypeUInt32,
                                                SphereIndexedBuffer[index],
                                                0);
}


void MTLEngine::CreateSkyBox()
{
    static const simd::float4 skyboxVerts[] = {
        
        { 1.0f, -1.0f,  1.0f, 1.0f}, { 1.0f, -1.0f, -1.0f, 1.0f}, { 1.0f,  1.0f, -1.0f, 1.0f},
        { 1.0f,  1.0f, -1.0f, 1.0f}, { 1.0f,  1.0f,  1.0f, 1.0f}, { 1.0f, -1.0f,  1.0f, 1.0f},
        
        {-1.0f, -1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f,  1.0f, 1.0f}, {-1.0f,  1.0f,  1.0f, 1.0f},
        {-1.0f,  1.0f,  1.0f, 1.0f}, {-1.0f,  1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f, 1.0f},
        
        {-1.0f,  1.0f,  1.0f, 1.0f}, { 1.0f,  1.0f,  1.0f, 1.0f}, { 1.0f,  1.0f, -1.0f, 1.0f},
        { 1.0f,  1.0f, -1.0f, 1.0f}, {-1.0f,  1.0f, -1.0f, 1.0f}, {-1.0f,  1.0f,  1.0f, 1.0f},
        
        {-1.0f, -1.0f, -1.0f, 1.0f}, { 1.0f, -1.0f, -1.0f, 1.0f}, { 1.0f, -1.0f,  1.0f, 1.0f},
        { 1.0f, -1.0f,  1.0f, 1.0f}, {-1.0f, -1.0f,  1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f, 1.0f},
        
        {-1.0f, -1.0f,  1.0f, 1.0f}, { 1.0f, -1.0f,  1.0f, 1.0f}, { 1.0f,  1.0f,  1.0f, 1.0f},
        { 1.0f,  1.0f,  1.0f, 1.0f}, {-1.0f,  1.0f,  1.0f, 1.0f}, {-1.0f, -1.0f,  1.0f, 1.0f},
        
        { 1.0f, -1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f, 1.0f}, {-1.0f,  1.0f, -1.0f, 1.0f},
        {-1.0f,  1.0f, -1.0f, 1.0f}, { 1.0f,  1.0f, -1.0f, 1.0f}, { 1.0f, -1.0f, -1.0f, 1.0f},
    };

    for(size_t i =0 ; i < kMaxDrawsPerFrame; i++){
        SkyBoxVertexBuffer[i] = metalDevice->newBuffer(
        skyboxVerts, sizeof(skyboxVerts), MTL::ResourceStorageModeShared);

        MVPSkyBoxBuffer[i] = metalDevice->newBuffer(sizeof(MVP), MTL::ResourceStorageModeShared);
    }

    const char *facePaths[6] = {
        "assets/Standard-Cube-Map/right.jpg",
        "assets/Standard-Cube-Map/left.jpg",
        "assets/Standard-Cube-Map/top.jpg",
        "assets/Standard-Cube-Map/bottom.jpg",
        "assets/Standard-Cube-Map/front.jpg",
        "assets/Standard-Cube-Map/back.jpg"};
    skyboxTexture = new CubeTexture(facePaths, metalDevice);


    MTL::SamplerDescriptor *samplerDescriptor = MTL::SamplerDescriptor::alloc()->init();
    samplerDescriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
    samplerDescriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
    samplerDescriptor->setMipFilter(MTL::SamplerMipFilterLinear);
    samplerDescriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
    samplerDescriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
    samplerDescriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
    samplerState = metalDevice->newSamplerState(samplerDescriptor);
    samplerDescriptor->release();

    SamplerBuffer = metalDevice->newBuffer(sizeof(MVP), MTL::ResourceStorageModeShared);

    memcpy(SamplerBuffer->contents(), &samplerState, sizeof(MTL::SamplerState *));
}

inline matrix_float4x4 MTLEngine::toSimd(const glm::mat4 &m)
{
    // glm::mat4 and matrix_float4x4 are both column-major float4x4 with
    // identical memory layout, so this is a straight reinterpret, not a copy loop.
    matrix_float4x4 result;
    memcpy(&result, glm::value_ptr(m), sizeof(matrix_float4x4));
    return result;
}

inline matrix_float3x3 MTLEngine::toSimd(const glm::mat3 &m)
{
    // glm::mat3 is tightly packed (3 floats/column) but matrix_float3x3 pads
    // each column to a float4, so this must be built column-by-column.
    return matrix_float3x3{
        simd::float3{m[0].x, m[0].y, m[0].z},
        simd::float3{m[1].x, m[1].y, m[1].z},
        simd::float3{m[2].x, m[2].y, m[2].z}
    };
}

inline simd::float3 MTLEngine::toSimd(const glm::vec3 &v)
{
    return simd::float3{v.x, v.y, v.z};
}