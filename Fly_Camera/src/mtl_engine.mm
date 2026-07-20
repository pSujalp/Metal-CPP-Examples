#include "mtl_engine.hpp"


static inline simd::float4x4 glmToSimd(const glm::mat4& m) {
    return simd::float4x4(
        simd::float4{m[0][0], m[0][1], m[0][2], m[0][3]},
        simd::float4{m[1][0], m[1][1], m[1][2], m[1][3]},
        simd::float4{m[2][0], m[2][1], m[2][2], m[2][3]},
        simd::float4{m[3][0], m[3][1], m[3][2], m[3][3]}
    );
}

void MTLEngine::init() {
    initDevice();
    initWindow();

    createCube();
    createBuffers();
    createDefaultLibrary();
    createCommandQueue();
    createRenderPipeline();
    createDepthAndMSAATextures();
    createRenderPassDescriptor();
}

void MTLEngine::run() {
    while (!glfwWindowShouldClose(glfwWindow)) {
        @autoreleasepool {
            metalDrawable = (__bridge CA::MetalDrawable*)[metalLayer nextDrawable];
            draw();
        }
        glfwPollEvents();
    }
}

void MTLEngine::cleanup() {
    
    
    cubeVertexBuffer->release();
    transformationBuffer->release();
    msaaRenderTargetTexture->release();
    depthTexture->release();
    renderPassDescriptor->release();
    depthStencilState->release();
    metalRenderPSO->release();
    metalDefaultLibrary->release();
    metalCommandQueue->release();
    metalDevice->release();
    delete grassTexture;
    delete camera;

    glfwTerminate();
}

void MTLEngine::initDevice() {
    metalDevice = MTL::CreateSystemDefaultDevice();
}

void MTLEngine::frameBufferSizeCallback(GLFWwindow *window, int width, int height) {
    MTLEngine* engine = (MTLEngine*)glfwGetWindowUserPointer(window);
    engine->resizeFrameBuffer(width, height);
}
void MTLEngine::resizeFrameBuffer(int width, int height) {
    metalLayer.drawableSize = CGSizeMake(width, height);
    
    if (msaaRenderTargetTexture) {
        msaaRenderTargetTexture->release();
        msaaRenderTargetTexture = nullptr;
    }
    if (depthTexture) {
        depthTexture->release();
        depthTexture = nullptr;
    }
    createDepthAndMSAATextures();
    metalDrawable = (__bridge CA::MetalDrawable*)[metalLayer nextDrawable];
    updateRenderPassDescriptor();
}

MTL::Library* MTLEngine::loadLibrary(MTL::Device* device, const char* path) {
    NS::Error* error = nullptr;

    NS::String* nsPath = NS::String::string(path, NS::StringEncoding::UTF8StringEncoding);
    NS::URL* url = NS::URL::fileURLWithPath(nsPath);

    MTL::Library* library = device->newLibrary(url, &error);

    if (!library) {
        printf("Failed to load library at %s: %s\n",
               path,
               error ? error->localizedDescription()->utf8String() : "unknown error");
        assert(false);
    }

    return library;
}

void MTLEngine::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow = glfwCreateWindow(800, 600, "Metal Engine", NULL, NULL);
    if (!glfwWindow) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow, frameBufferSizeCallback);
    glfwSetCursorPosCallback(glfwWindow, mouse_callback);
    glfwSetScrollCallback(glfwWindow, scroll_callback);
    glfwSetMouseButtonCallback(glfwWindow, mouse_button_callback);

    int width, height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f));
    lastX = width / 2.0f;
    lastY = height / 2.0f;

    metalWindow = glfwGetCocoaWindow(glfwWindow);
    metalLayer = [CAMetalLayer layer];
    metalLayer.device = (__bridge id<MTLDevice>)metalDevice;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.drawableSize = CGSizeMake(width, height);
    metalWindow.contentView.layer = metalLayer;
    metalWindow.contentView.wantsLayer = YES;

    metalDrawable = (__bridge CA::MetalDrawable*)[metalLayer nextDrawable];
}

void MTLEngine::createCube() {
    VertexData cubeVertices[] = {
        
        {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}},
        {{0.5, -0.5, 0.5, 1.0}, {1.0, 0.0}},
        {{0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}},
        {{0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 1.0}},
        {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}},

        
        {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
        {{-0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
        {{-0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{-0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}},
        {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

        
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}},
        {{0.5, 0.5, 0.5, 1.0}, {1.0, 0.0}},
        {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{-0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}},

        
        {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
        {{0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
        {{0.5, -0.5, 0.5, 1.0}, {1.0, 1.0}},
        {{0.5, -0.5, 0.5, 1.0}, {1.0, 1.0}},
        {{-0.5, -0.5, 0.5, 1.0}, {0.0, 1.0}},
        {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

        
        {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
        {{-0.5, -0.5, 0.5, 1.0}, {1.0, 0.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}},
        {{-0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}},
        {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

        
        {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}},
        {{0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
        {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{0.5, 0.5, 0.5, 1.0}, {0.0, 1.0}},
        {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}},
    };

    cubeVertexBuffer = metalDevice->newBuffer(&cubeVertices, sizeof(cubeVertices), MTL::ResourceStorageModeShared);

    
    
    grassTexture = new Texture("assets/mc_grass.jpeg", metalDevice);
}

void MTLEngine::createBuffers() {
    transformationBuffer = metalDevice->newBuffer(sizeof(TransformationData), MTL::ResourceStorageModeShared);
}

void MTLEngine::createDefaultLibrary() {
    Shader sh;
    std::filesystem::path exeDir = sh.executableDirectory();

    std::string dirStr = exeDir.string();
    dirStr.append("/default.metallib");

    const char* dirCStr = dirStr.c_str();
    metalDefaultLibrary = loadLibrary(metalDevice, dirCStr);

    if (!metalDefaultLibrary) {
        std::cerr << "Failed to load default library.";
        std::exit(-1);
    }
}

void MTLEngine::createCommandQueue() {
    metalCommandQueue = metalDevice->newCommandQueue();
}

void MTLEngine::createRenderPipeline() {
    MTL::Function* vertexShader = metalDefaultLibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function* fragmentShader = metalDefaultLibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer.pixelFormat;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->setSampleCount(sampleCount);
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    NS::Error* error;
    metalRenderPSO = metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);

    if (metalRenderPSO == nil) {
        std::cout << "Error creating render pipeline state: " << error << std::endl;
        std::exit(0);
    }

    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = metalDevice->newDepthStencilState(depthStencilDescriptor);

    renderPipelineDescriptor->release();
    depthStencilDescriptor->release();
    vertexShader->release();
    fragmentShader->release();
}

void MTLEngine::createDepthAndMSAATextures() {
    MTL::TextureDescriptor* msaaTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    msaaTextureDescriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    msaaTextureDescriptor->setWidth(metalLayer.drawableSize.width);
    msaaTextureDescriptor->setHeight(metalLayer.drawableSize.height);
    msaaTextureDescriptor->setSampleCount(sampleCount);
    msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);

    msaaRenderTargetTexture = metalDevice->newTexture(msaaTextureDescriptor);

    MTL::TextureDescriptor* depthTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
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

void MTLEngine::createRenderPassDescriptor() {
    renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

    MTL::RenderPassColorAttachmentDescriptor* colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
    MTL::RenderPassDepthAttachmentDescriptor* depthAttachment = renderPassDescriptor->depthAttachment();

    colorAttachment->setTexture(msaaRenderTargetTexture);
    colorAttachment->setResolveTexture(metalDrawable->texture());
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setClearColor(MTL::ClearColor(41.0f / 255.0f, 42.0f / 255.0f, 48.0f / 255.0f, 1.0));
    colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);

    depthAttachment->setTexture(depthTexture);
    depthAttachment->setLoadAction(MTL::LoadActionClear);
    depthAttachment->setStoreAction(MTL::StoreActionDontCare);
    depthAttachment->setClearDepth(1.0);
}

void MTLEngine::updateRenderPassDescriptor() {
    renderPassDescriptor->colorAttachments()->object(0)->setTexture(msaaRenderTargetTexture);
    renderPassDescriptor->colorAttachments()->object(0)->setResolveTexture(metalDrawable->texture());
    renderPassDescriptor->depthAttachment()->setTexture(depthTexture);
}

void MTLEngine::draw() {
    
    
    processInput(glfwWindow);

    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    sendRenderCommand();
}

void MTLEngine::sendRenderCommand() {
    metalCommandBuffer = metalCommandQueue->commandBuffer();

    updateRenderPassDescriptor();
    MTL::RenderCommandEncoder* renderCommandEncoder = metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);
    encodeRenderCommand(renderCommandEncoder);
    renderCommandEncoder->endEncoding();

    metalCommandBuffer->presentDrawable(metalDrawable);
    metalCommandBuffer->commit();
    
    
    
    
    
    
}

void MTLEngine::encodeRenderCommand(MTL::RenderCommandEncoder* renderCommandEncoder) {
    
    
    

    
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));

    float angleInDegrees = static_cast<float>(glfwGetTime()) / 2.0f * 45.0f;
    float angleInRadians = glm::radians(angleInDegrees);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angleInRadians, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 modelMatrix = translationMatrix * rotationMatrix;


    glm::mat4 viewMatrix = camera->GetViewMatrix();
    float nearZ = 0.1f;
    float farZ = 100.0f;

    glm::mat4 perspectiveMatrix = glm::perspective(glm::radians(camera->Zoom), (float)800 / (float)600, nearZ, farZ);

    
    
    
    TransformationData transformationData = {
        glmToSimd(modelMatrix),
        glmToSimd(viewMatrix),
        glmToSimd(perspectiveMatrix)
    };
    memcpy(transformationBuffer->contents(), &transformationData, sizeof(transformationData));

    renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    renderCommandEncoder->setCullMode(MTL::CullModeBack);
    renderCommandEncoder->setRenderPipelineState(metalRenderPSO);
    renderCommandEncoder->setDepthStencilState(depthStencilState);
    renderCommandEncoder->setVertexBuffer(cubeVertexBuffer, 0, 0);
    renderCommandEncoder->setVertexBuffer(transformationBuffer, 0, 1);
    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
    NS::UInteger vertexStart = 0;
    NS::UInteger vertexCount = 36;
    renderCommandEncoder->setFragmentTexture(grassTexture->texture, 0);
    renderCommandEncoder->drawPrimitives(typeTriangle, vertexStart, vertexCount);
}

void MTLEngine::processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->ProcessKeyboard(RIGHT, deltaTime);

    
}



void MTLEngine::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    MTLEngine* engine = (MTLEngine*)glfwGetWindowUserPointer(window);
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        
            

            int width, height;
            glfwGetWindowSize(window, &width, &height);
            double xpos = width / 2.0;
            double ypos = height / 2.0;
            glfwSetCursorPos(window, xpos, ypos);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            
            engine->lastX = static_cast<float>(xpos);
            engine->lastY = static_cast<float>(ypos);
    } 
    else{

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    }
}
void MTLEngine::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    
    
    
    
    MTLEngine* engine = (MTLEngine*)glfwGetWindowUserPointer(window);
    if (!engine) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (engine->firstMouse)
    {
        engine->lastX = xpos;
        engine->lastY = ypos;
        engine->firstMouse = false;
    }

    float xoffset = xpos - engine->lastX;
    float yoffset = engine->lastY - ypos; 

    engine->lastX = xpos;
    engine->lastY = ypos;

     engine->camera->ProcessMouseMovement(xoffset, yoffset);
}

void MTLEngine::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    MTLEngine* engine = (MTLEngine*)glfwGetWindowUserPointer(window);
    if (!engine) return;
    engine->camera->ProcessMouseScroll(static_cast<float>(yoffset));
}