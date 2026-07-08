
#include "mtl_engine.hpp"

void MTLEngine::init() {
    initDevice();
    initWindow();

    

    createTriangle();
    createCommandQueue();
    createRenderPipeline();
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
    glfwTerminate();
    metalDevice->release();
}

void MTLEngine::initDevice() {
    metalDevice = MTL::CreateSystemDefaultDevice();
}

void MTLEngine::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow = glfwCreateWindow(800, 600, "Metal Engine", NULL, NULL);
    
    if (!glfwWindow) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    int width, height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    
    metalWindow = glfwGetCocoaWindow(glfwWindow);
    metalLayer = [CAMetalLayer layer];
    metalLayer.device = (__bridge id<MTLDevice>)metalDevice;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.drawableSize = CGSizeMake(width, height);
    metalWindow.contentView.layer = metalLayer;
    metalWindow.contentView.wantsLayer = YES;
}

void MTLEngine::createTriangle() {
    simd::float3 triangleVertices[] = {
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.0f,  0.5f, 0.0f}
    };
    
    triangleVertexBuffer = metalDevice->newBuffer(&triangleVertices, sizeof(triangleVertices), MTL::ResourceStorageModeShared);
}




void MTLEngine::createCommandQueue() {
    metalCommandQueue = metalDevice->newCommandQueue();
    metal4CommandQueue = metalDevice->newMTL4CommandQueue();
}

void MTLEngine::createRenderPipeline() {
    
    using NS::StringEncoding::UTF8StringEncoding;

    Shader sh;
    NS::Error* pError = nullptr;

    MTL::Library* pLibrary = metalDevice->newLibrary( NS::String::string(sh.GetShader("shaders/shaders.metal"), UTF8StringEncoding), nullptr, &pError );
    
    MTL::Function* vertexShader = pLibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function* fragmentShader = pLibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);
    
    MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setLabel(NS::String::string("Triangle Rendering Pipeline", NS::ASCIIStringEncoding));
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer.pixelFormat;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);

    shader_lib = metalDevice->newDefaultLibrary();
    shader_lib = metalDevice->newLibrary( NS::String::string( "shaders/shaders.metallib" , NS::ASCIIStringEncoding ), nullptr );

    MTL4::Compiler* compiler;
    {
            auto* compiler_desc = MTL4::CompilerDescriptor::alloc()->init();

            compiler = metalDevice->newCompiler( compiler_desc, nullptr );
    }

     auto* vertex_fun_desc = MTL4::LibraryFunctionDescriptor::alloc()->init();
     vertex_fun_desc->setLibrary( shader_lib );
     vertex_fun_desc->setName( NS::String::string( "vertexShader" , NS::ASCIIStringEncoding ) );

    auto* fragment_fun_desc = MTL4::LibraryFunctionDescriptor::alloc()->init();
    fragment_fun_desc->setLibrary( shader_lib );
    fragment_fun_desc->setName( NS::String::string( "fragmentShader" , NS::ASCIIStringEncoding ) );

    auto* desc = MTL4::RenderPipelineDescriptor::alloc()->init();

    desc->setLabel(NS::String::string("Triangle Rendering Pipeline", NS::ASCIIStringEncoding) );
    desc->colorAttachments()->object( 0 )->setPixelFormat((MTL::PixelFormat)metalLayer.pixelFormat );
    desc->setVertexFunctionDescriptor( vertex_fun_desc );
    desc->setFragmentFunctionDescriptor( fragment_fun_desc );
    metalRenderPSO1 = compiler->newRenderPipelineState( desc, (MTL4::CompilerTaskOptions*)nullptr, (NS::Error**)nullptr );
    

    NS::Error* error;
    metalRenderPSO = metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);
    
    renderPipelineDescriptor->release();
}

void MTLEngine::draw() {
    sendRenderCommand();
 
}

void MTLEngine::sendRenderCommand() {
    metalCommandBuffer = metalCommandQueue->commandBuffer();
    metal4CommandBuffer = metalDevice->newCommandBuffer();
    
    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL4::RenderPassDescriptor * renderPassDescriptor_M4 = MTL4::RenderPassDescriptor::alloc()->init();

    MTL::RenderPassColorAttachmentDescriptor* cd = renderPassDescriptor->colorAttachments()->object(0);
    MTL::RenderPassColorAttachmentDescriptor* cd1 = renderPassDescriptor_M4->colorAttachments()->object(0);


    cd->setTexture(metalDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);


    cd1->setTexture(metalDrawable->texture());
    cd1->setLoadAction(MTL::LoadActionClear);
    cd1->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    cd1->setStoreAction(MTL::StoreActionStore);

    MTL4::RenderCommandEncoder* renderCommandEncoderM_4 = metal4CommandBuffer->renderCommandEncoder(renderPassDescriptor_M4);
    MTL::RenderCommandEncoder* renderCommandEncoder = metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);


    encodeRenderCommand(renderCommandEncoder);
    encodeRenderCommand_M4(renderCommandEncoderM_4);
    
    renderCommandEncoder->endEncoding();

    metalCommandBuffer->presentDrawable(metalDrawable);
    metalCommandBuffer->commit();
    metalCommandBuffer->waitUntilCompleted();
    
    renderPassDescriptor->release();

}

void MTLEngine::encodeRenderCommand(MTL::RenderCommandEncoder* renderCommandEncoder) {
    renderCommandEncoder->setRenderPipelineState(metalRenderPSO);
    renderCommandEncoder->setVertexBuffer(triangleVertexBuffer, 0, 0);
    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
    NS::UInteger vertexStart = 0;
    NS::UInteger vertexCount = 3;
    renderCommandEncoder->drawPrimitives(typeTriangle, vertexStart, vertexCount);
}

void MTLEngine::encodeRenderCommand_M4(MTL4::RenderCommandEncoder* renderCommandEncoder) {

    renderCommandEncoder->setRenderPipelineState(metalRenderPSO);
    renderCommandEncoder->setVertexBuffer(triangleVertexBuffer, 0, 0);
    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
    NS::UInteger vertexStart = 0;
    NS::UInteger vertexCount = 3;
    renderCommandEncoder->drawPrimitives(typeTriangle, vertexStart, vertexCount);


}