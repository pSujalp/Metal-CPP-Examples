#include "Renderer.hpp"
#include "VertexData.hpp"
#include "Shader.h"

Renderer::Renderer(MTL::Device* pDevice)
: _pDevice(pDevice->retain())
{
    _pCommandQueue = _pDevice->newCommandQueue();
    createDefaultLibrary(pDevice);
    buildShaders();
    CreateCube();
}

Renderer::~Renderer()
{
    cubeVertexBuffer->release();
    delete grassTexture;
    _pPSO->release();
    _renderToTexturePipelineState->release();
    _renderTexture->release();
    _offscreenDepthTexture->release();
    _renderToTextureRenderPassDescriptor->release();
    depthStencilState->release();
    transformationBuffer->release();
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::createDefaultLibrary(MTL::Device* pDevice) {
    using NS::StringEncoding::UTF8StringEncoding;
    Shader sh;
    const char* shadersrc = sh.GetShader("shaders/square.metal");
    assert(shadersrc);
    NS::Error* pError = nullptr;
    metallibrary = pDevice->newLibrary(
        NS::String::string(shadersrc, UTF8StringEncoding), nullptr, &pError
    );
    if (!metallibrary) {
        __builtin_printf("Compile FAILED: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }
}

void Renderer::CreateCube() {
    VertexData cubeVertices[] = {
        {{-0.5, -0.5,  0.5, 1.0}, {0.0, 0.0}},
        {{ 0.5, -0.5,  0.5, 1.0}, {1.0, 0.0}},
        {{ 0.5,  0.5,  0.5, 1.0}, {1.0, 1.0}},
        {{ 0.5,  0.5,  0.5, 1.0}, {1.0, 1.0}},
        {{-0.5,  0.5,  0.5, 1.0}, {0.0, 1.0}},
        {{-0.5, -0.5,  0.5, 1.0}, {0.0, 0.0}},

        {{ 0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
        {{-0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
        {{-0.5,  0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{-0.5,  0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{ 0.5,  0.5, -0.5, 1.0}, {0.0, 1.0}},
        {{ 0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

        {{-0.5,  0.5,  0.5, 1.0}, {0.0, 0.0}},
        {{ 0.5,  0.5,  0.5, 1.0}, {1.0, 0.0}},
        {{ 0.5,  0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{ 0.5,  0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{-0.5,  0.5, -0.5, 1.0}, {0.0, 1.0}},
        {{-0.5,  0.5,  0.5, 1.0}, {0.0, 0.0}},

        {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
        {{ 0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
        {{ 0.5, -0.5,  0.5, 1.0}, {1.0, 1.0}},
        {{ 0.5, -0.5,  0.5, 1.0}, {1.0, 1.0}},
        {{-0.5, -0.5,  0.5, 1.0}, {0.0, 1.0}},
        {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

        {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
        {{-0.5, -0.5,  0.5, 1.0}, {1.0, 0.0}},
        {{-0.5,  0.5,  0.5, 1.0}, {1.0, 1.0}},
        {{-0.5,  0.5,  0.5, 1.0}, {1.0, 1.0}},
        {{-0.5,  0.5, -0.5, 1.0}, {0.0, 1.0}},
        {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

        {{ 0.5, -0.5,  0.5, 1.0}, {0.0, 0.0}},
        {{ 0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
        {{ 0.5,  0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{ 0.5,  0.5, -0.5, 1.0}, {1.0, 1.0}},
        {{ 0.5,  0.5,  0.5, 1.0}, {0.0, 1.0}},
        {{ 0.5, -0.5,  0.5, 1.0}, {0.0, 0.0}},
    };
    cubeVertexBuffer = _pDevice->newBuffer(
        &cubeVertices, sizeof(cubeVertices), MTL::ResourceStorageModeShared
    );
    grassTexture = new Texture("assets/mc_grass.jpeg", _pDevice);
}

void Renderer::buildShaders()
{
    NS::Error* pError = nullptr;

    
    MTL::TextureDescriptor* colorDesc = MTL::TextureDescriptor::alloc()->init();
    colorDesc->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    colorDesc->setWidth(512);
    colorDesc->setHeight(512);
    colorDesc->setStorageMode(MTL::StorageModeShared);
    colorDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    _renderTexture = _pDevice->newTexture(colorDesc);
    colorDesc->release();

    
    MTL::TextureDescriptor* depthDesc = MTL::TextureDescriptor::alloc()->init();
    depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthDesc->setWidth(512);
    depthDesc->setHeight(512);
    depthDesc->setStorageMode(MTL::StorageModePrivate);
    depthDesc->setUsage(MTL::TextureUsageRenderTarget);
    _offscreenDepthTexture = _pDevice->newTexture(depthDesc);
    depthDesc->release();

    
    _renderToTextureRenderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    _renderToTextureRenderPassDescriptor->colorAttachments()->object(0)->setTexture(_renderTexture);
    _renderToTextureRenderPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
    _renderToTextureRenderPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    _renderToTextureRenderPassDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(0.1, 0.1, 0.1, 1.0));
    _renderToTextureRenderPassDescriptor->depthAttachment()->setTexture(_offscreenDepthTexture);
    _renderToTextureRenderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    _renderToTextureRenderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    _renderToTextureRenderPassDescriptor->depthAttachment()->setClearDepth(1.0);

    
    MTL::Function* vertexFn = metallibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    assert(vertexFn);
    MTL::Function* fragmentFn = metallibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentFn);

    MTL::RenderPipelineDescriptor* pDesc1 = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc1->setVertexFunction(vertexFn);
    pDesc1->setFragmentFunction(fragmentFn);
    pDesc1->colorAttachments()->object(0)->setPixelFormat(_renderTexture->pixelFormat());
    pDesc1->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    _renderToTexturePipelineState = _pDevice->newRenderPipelineState(pDesc1, &pError);
    assert(_renderToTexturePipelineState);
    vertexFn->release();
    fragmentFn->release();
    pDesc1->release();

    MTL::DepthStencilDescriptor* depthStencilDesc = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthStencilDesc->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(depthStencilDesc);
    depthStencilDesc->release();
    transformationBuffer = _pDevice->newBuffer(sizeof(MVP), MTL::ResourceStorageModeShared);

    
    MTL::Function* vertexRPFn = metallibrary->newFunction(NS::String::string("vertexRenderPass", NS::ASCIIStringEncoding));
    assert(vertexRPFn);
    MTL::Function* fragmentRPFn = metallibrary->newFunction(NS::String::string("fragmentRenderPass", NS::ASCIIStringEncoding));
    assert(fragmentRPFn);

    MTL::RenderPipelineDescriptor* pDesc2 = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc2->setVertexFunction(vertexRPFn);
    pDesc2->setFragmentFunction(fragmentRPFn);
    pDesc2->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    _pPSO = _pDevice->newRenderPipelineState(pDesc2, &pError);
    assert(_pPSO);
    vertexRPFn->release();
    fragmentRPFn->release();
    pDesc2->release();
}

void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();


    {
        MTL::RenderPassDescriptor* pRpd2 = pView->currentRenderPassDescriptor();
        MTL::RenderCommandEncoder* pEnc2 = pCmd->renderCommandEncoder(pRpd2);
        pEnc2->setRenderPipelineState(_pPSO);

        static const AAPLVertex quadVertices[] = {
            {{-1.0, -1.0}, {0.0, 0.0, 0.0, 1.0}, {0.0, 1.0}},
            {{ 1.0, -1.0}, {1.0, 0.0, 0.0, 1.0}, {1.0, 1.0}},
            {{ 1.0,  1.0}, {1.0, 1.0, 0.0, 1.0}, {1.0, 0.0}},
            {{ 1.0,  1.0}, {1.0, 1.0, 0.0, 1.0}, {1.0, 0.0}},
            {{-1.0,  1.0}, {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0}},
            {{-1.0, -1.0}, {0.0, 0.0, 0.0, 1.0}, {0.0, 1.0}},
        };
        pEnc2->setVertexBytes(quadVertices, sizeof(quadVertices), 0);
        pEnc2->setFragmentTexture(_renderTexture, 0);
        pEnc2->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));
        pEnc2->endEncoding();
    }


    
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 1.0f, -1.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

        static float accumulatedDegrees = 0.0f;
        const float rotationSpeedDegreesPerSecond = 45.0f;
        accumulatedDegrees += rotationSpeedDegreesPerSecond * Time::DeltaTime;
        if (accumulatedDegrees >= 360.0f) accumulatedDegrees -= 360.0f;

        model = glm::rotate(model, accumulatedDegrees * (float)(M_PI / 180.0), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 viewMatrix = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        glm::mat4 perspectiveMatrix = glm::perspective(glm::radians(60.0f), 16.0f/9.0f, 0.1f, 1000.0f);
        glm::mat4 MVP_GLM = perspectiveMatrix * viewMatrix * model;

MVP mvp1;
mvp1.modelViewProjection = matrix_float4x4({
    simd::float4{ MVP_GLM[0][0], MVP_GLM[0][1], MVP_GLM[0][2], MVP_GLM[0][3] },
    simd::float4{ MVP_GLM[1][0], MVP_GLM[1][1], MVP_GLM[1][2], MVP_GLM[1][3] },
    simd::float4{ MVP_GLM[2][0], MVP_GLM[2][1], MVP_GLM[2][2], MVP_GLM[2][3] },
    simd::float4{ MVP_GLM[3][0], MVP_GLM[3][1], MVP_GLM[3][2], MVP_GLM[3][3] },
});
memcpy(transformationBuffer->contents(), &mvp1, sizeof(MVP));
        memcpy(transformationBuffer->contents(), &mvp1, sizeof(MVP));

        MTL::RenderCommandEncoder* pEnc1 = pCmd->renderCommandEncoder(_renderToTextureRenderPassDescriptor);
        pEnc1->setRenderPipelineState(_renderToTexturePipelineState);
        pEnc1->setDepthStencilState(depthStencilState);
        pEnc1->setVertexBuffer(cubeVertexBuffer,    0, 0);
        pEnc1->setVertexBuffer(transformationBuffer, 0, 1);
        pEnc1->setFragmentTexture(grassTexture->texture, 0);
        pEnc1->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(36));
        pEnc1->endEncoding();
    }

    
    
    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();
    pPool->release();
}
