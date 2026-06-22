#include "Renderer.hpp"
#include "VertexData.hpp"
#include "Shader.h"







Renderer::Renderer(MTL::Device* pDevice)
: _pDevice(pDevice->retain())
{
    __builtin_printf("Step 1: creating command queue\n");
    _pCommandQueue = _pDevice->newCommandQueue();

    __builtin_printf("Step 2: createDefaultLibrary\n");
    createDefaultLibrary(pDevice);

    __builtin_printf("Step 3: buildShaders\n");
    buildShaders();

    __builtin_printf("Step 4: buildSkyBoxShaders\n");
    buildSkyBoxShaders();

    __builtin_printf("Step 5: CreateCube\n");
    CreateCube();

    __builtin_printf("Step 6: CreateSkyBox\n");
    CreateSkyBox();

    __builtin_printf("Constructor done\n");
}


Renderer::~Renderer()
{
    cubeVertexBuffer->release();
    delete grassTexture;
    _pPSO->release();
    depthStencilState->release();
    UniformBuffer->release();
    transformationBuffer->release();

    SkyBoxVertexBuffer->release();
    MVPSkyBoxBuffer->release();
    _SkyboxPSO->release();
    SkyBoxDepthStencilState->release();
    delete skyboxTexture;

    metallibrary->release();
    metalSkyBoxlibrary->release();

    _pCommandQueue->release();
    _pDevice->release();
}


void Renderer::createDefaultLibrary(MTL::Device* pDevice)
{
    using NS::StringEncoding::UTF8StringEncoding;

    {
        Shader sh;
        const char* src = sh.GetShader("shaders/square.metal");
        assert(src && "ERROR: shaders/square.metal not found");

        NS::Error* pError = nullptr;
        metallibrary = pDevice->newLibrary(
            NS::String::string(src, UTF8StringEncoding), nullptr, &pError
        );
        if (!metallibrary) {
            __builtin_printf("Cube shader compile FAILED: %s\n",
                pError->localizedDescription()->utf8String());
            assert(false);
        }
        __builtin_printf("Cube shader library OK\n");
    }

    {
        Shader sh;
        const char* src = sh.GetShader("shaders/skybox.metal");
        assert(src && "ERROR: shaders/skybox.metal not found");

        NS::Error* pError = nullptr;
        metalSkyBoxlibrary = pDevice->newLibrary(
            NS::String::string(src, UTF8StringEncoding), nullptr, &pError
        );
        if (!metalSkyBoxlibrary) {
            __builtin_printf("Skybox shader compile FAILED: %s\n",
                pError->localizedDescription()->utf8String());
            assert(false);
        }
        __builtin_printf("Skybox shader library OK\n");
    }
}


void Renderer::buildShaders()
{
    NS::Error* pError = nullptr;

    MTL::Function* vert = metallibrary->newFunction(
        NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    assert(vert && "ERROR: 'vertexShader' not found");

    MTL::Function* frag = metallibrary->newFunction(
        NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(frag && "ERROR: 'fragmentShader' not found");

    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(vert);
    pDesc->setFragmentFunction(frag);
    pDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
    depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthDesc->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(depthDesc);
    depthDesc->release();

    _pPSO = _pDevice->newRenderPipelineState(pDesc, &pError);
    if (!_pPSO) {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    UniformBuffer        = _pDevice->newBuffer(sizeof(Uniforms), MTL::ResourceStorageModeShared);
    transformationBuffer = _pDevice->newBuffer(sizeof(MVP),      MTL::ResourceStorageModeShared);

    frag->release();
    vert->release();
    pDesc->release();
}


void Renderer::buildSkyBoxShaders()
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
    pDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
    depthDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthDesc->setDepthWriteEnabled(false);
    SkyBoxDepthStencilState = _pDevice->newDepthStencilState(depthDesc);
    depthDesc->release();

    _SkyboxPSO = _pDevice->newRenderPipelineState(pDesc, &pError);
    if (!_SkyboxPSO) {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    frag->release();
    vert->release();
    pDesc->release();
}


void Renderer::CreateCube()
{
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
        cubeVertices, sizeof(cubeVertices), MTL::ResourceStorageModeShared
    );
    grassTexture = new Texture("assets/mc_grass.jpeg", _pDevice);
}


void Renderer::CreateSkyBox()
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

    SkyBoxVertexBuffer = _pDevice->newBuffer(
        skyboxVerts, sizeof(skyboxVerts), MTL::ResourceStorageModeShared
    );
    MVPSkyBoxBuffer = _pDevice->newBuffer(sizeof(MVP), MTL::ResourceStorageModeShared);

    skyboxTexture = new Texture("assets/skybox.png", _pDevice);
}


void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    
    Uniforms uniforms;
    uniforms.time = {0.1f, 0.3f};
    memcpy(UniformBuffer->contents(), &uniforms, sizeof(Uniforms));

    
    glm::mat4 viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    auto  drawableSize = pView->drawableSize();
    float aspect       = (float)drawableSize.width / (float)drawableSize.height;
    glm::mat4 proj     = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.0f, -1.0f));
    static float deg = 0.0f;
    deg += 45.0f * Time::DeltaTime;
    if (deg >= 360.0f) deg -= 360.0f;
    model = glm::rotate(model, glm::radians(deg), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 MVP_GLM = proj * viewMatrix * model;
    MVP mvp1;
    mvp1.MVP = matrix_float4x4({
        simd::float4{ MVP_GLM[0][0], MVP_GLM[0][1], MVP_GLM[0][2], MVP_GLM[0][3] },
        simd::float4{ MVP_GLM[1][0], MVP_GLM[1][1], MVP_GLM[1][2], MVP_GLM[1][3] },
        simd::float4{ MVP_GLM[2][0], MVP_GLM[2][1], MVP_GLM[2][2], MVP_GLM[2][3] },
        simd::float4{ MVP_GLM[3][0], MVP_GLM[3][1], MVP_GLM[3][2], MVP_GLM[3][3] },
    });
    memcpy(transformationBuffer->contents(), &mvp1, sizeof(MVP));

    
    static float skyboxDeg = 0.0f;
    skyboxDeg += 1.0f * Time::DeltaTime;
    if (skyboxDeg >= 360.0f) skyboxDeg -= 360.0f;

    glm::mat4 skyboxModel = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
    skyboxModel = glm::rotate(skyboxModel, glm::radians(skyboxDeg), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 skyboxMVP_GLM = proj * viewMatrix * skyboxModel;

    MVP mvpSkybox;
    mvpSkybox.MVP = matrix_float4x4({
        simd::float4{ skyboxMVP_GLM[0][0], skyboxMVP_GLM[0][1], skyboxMVP_GLM[0][2], skyboxMVP_GLM[0][3] },
        simd::float4{ skyboxMVP_GLM[1][0], skyboxMVP_GLM[1][1], skyboxMVP_GLM[1][2], skyboxMVP_GLM[1][3] },
        simd::float4{ skyboxMVP_GLM[2][0], skyboxMVP_GLM[2][1], skyboxMVP_GLM[2][2], skyboxMVP_GLM[2][3] },
        simd::float4{ skyboxMVP_GLM[3][0], skyboxMVP_GLM[3][1], skyboxMVP_GLM[3][2], skyboxMVP_GLM[3][3] },
    });
    memcpy(MVPSkyBoxBuffer->contents(), &mvpSkybox, sizeof(MVP));

    
    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    if (!pCmd) {
        __builtin_printf("ERROR: commandBuffer is nil\n");
        pPool->release();
        return;
    }

    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    if (!pRpd) {
        __builtin_printf("ERROR: currentRenderPassDescriptor is nil\n");
        pCmd->commit();
        pPool->release();
        return;
    }

    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder(pRpd);
    if (!pEnc) {
        __builtin_printf("ERROR: renderCommandEncoder is nil\n");
        pCmd->commit();
        pPool->release();
        return;
    }

    
    pEnc->setRenderPipelineState(_SkyboxPSO);
    pEnc->setDepthStencilState(SkyBoxDepthStencilState);
    pEnc->setVertexBuffer(SkyBoxVertexBuffer, 0, 0);  
    pEnc->setVertexBuffer(MVPSkyBoxBuffer,    0, 2);  
    pEnc->setFragmentTexture(skyboxTexture->texture, 0);
    pEnc->drawPrimitives(MTL::PrimitiveTypeTriangle,  
                         NS::UInteger(0), NS::UInteger(36));

    
    pEnc->setRenderPipelineState(_pPSO);
    pEnc->setDepthStencilState(depthStencilState);
    pEnc->setVertexBuffer(cubeVertexBuffer,     0, 0);  
    pEnc->setVertexBuffer(UniformBuffer,        0, 1);  
    pEnc->setVertexBuffer(transformationBuffer, 0, 2);  
    pEnc->setFragmentTexture(grassTexture->texture, 0);
    pEnc->drawPrimitives(MTL::PrimitiveTypeTriangle,
                         NS::UInteger(0), NS::UInteger(36));

    pEnc->endEncoding();
    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();

    pPool->release();
}