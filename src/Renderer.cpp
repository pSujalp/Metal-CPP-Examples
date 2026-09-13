#include "Renderer.hpp"
#include "Shader.h"

Renderer::Renderer(MTL::Device *pDevice)
    : _pDevice(pDevice->retain())
{
    _pCommandQueue = _pDevice->newCommandQueue();

    createRenderPipeline();
    createLightSourceRenderPipeline();
    loadMeshes();
}

Renderer::~Renderer()
{

    lightVertexBuffer->release();
    metalRenderPSO->release();
    metalLightSourceRenderPSO->release();
    depthStencilState->release();
    shadowPipelineState->release();
    shadowTexture->release();
    shadowRenderPassDescriptor->release();
    delete model;
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::draw(MTK::View *pView)
{
    NS::AutoreleasePool *pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer *pCmd = _pCommandQueue->commandBuffer();

    matrix_float4x4 rotationMatrix = matrix4x4_rotation(60 * (M_PI / 180.0f), 0.0, 1.0, 0.0);

    
    matrix_float4x4 viewMatrix = matrix_look_at_right_hand(
        simd_make_float3(0.0f, 0.0f, 0.0f),  
        simd_make_float3(0.0f, 0.0f, -1.0f), 
        simd_make_float3(0.0f, 1.0f, 0.0f)   
    );

    simd_float4 lightColor = simd_make_float4(1.0f, 1.0f, 1.0f, 1.0f);
    simd_float4 lightPosition = simd_make_float4(-0.5f, -0.0f, 0.0f, -1.0f);
    simd_float3 cameraPosition = simd_make_float3(-1.0f, 0.0f, 0.0f);

    auto drawableSize = pView->drawableSize();
    float aspectRatio = (float)(drawableSize.width / drawableSize.height);
    float fov = 45.0f * (M_PI / 180.0f);
    float nearZ = 0.1f;
    float farZ = 1000.0f;
    matrix_float4x4 perspectiveMatrix = matrix_perspective_right_hand(fov, aspectRatio, nearZ, farZ);

    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;

    
    matrix_float4x4 scaleMatrix = matrix4x4_scale(12.01f, 12.01f, 12.01f);
    matrix_float4x4 modelMatrix = matrix4x4_translation(0.0f, -0.5f, -5.0f) * rotationMatrix * scaleMatrix;

    matrix_float4x4 smallScaleMatrix = matrix4x4_scale(4.01f, 4.01f, 4.01f);
    matrix_float4x4 smallModelMatrix = matrix4x4_translation(-0.3f, -0.25f, -2.5f) * rotationMatrix * smallScaleMatrix;

    
    
    
    
    simd_float3 lightPos3 = simd_make_float3(lightPosition[0], lightPosition[1], lightPosition[2]);
    simd_float3 sceneCenter = simd_make_float3(0.0f, -0.25f, -4.0f);
    matrix_float4x4 lightViewMatrix = matrix_look_at_right_hand(lightPos3, sceneCenter, simd_make_float3(0.0f, 1.0f, 0.0f));
    matrix_float4x4 lightProjectionMatrix = matrix_perspective_right_hand(75.0f * (M_PI / 180.0f), 1.0f, 0.5f, 200.0f);
    matrix_float4x4 lightViewProjectionMatrix = lightProjectionMatrix * lightViewMatrix;

    
    MTL::RenderCommandEncoder *pShadowEnc = pCmd->renderCommandEncoder(shadowRenderPassDescriptor);
    pShadowEnc->setRenderPipelineState(shadowPipelineState);
    pShadowEnc->setDepthStencilState(depthStencilState);
    pShadowEnc->setFrontFacingWinding(MTL::WindingCounterClockwise); 
    pShadowEnc->setCullMode(MTL::CullModeBack);
    pShadowEnc->setDepthBias(1.0f, 1.0f, 0.01f);

    for (Mesh *mesh : model->meshes)
    {
        pShadowEnc->setVertexBuffer(mesh->vertexBuffer, 0, 0);
        pShadowEnc->setVertexBytes(&modelMatrix, sizeof(modelMatrix), 1);
        pShadowEnc->setVertexBytes(&lightViewProjectionMatrix, sizeof(lightViewProjectionMatrix), 2);
        pShadowEnc->drawIndexedPrimitives(typeTriangle,
                                          mesh->indexCount,
                                          MTL::IndexTypeUInt32,
                                          mesh->indexBuffer,
                                          0);
    }
    for (Mesh *mesh : model->meshes)
    {
        pShadowEnc->setVertexBuffer(mesh->vertexBuffer, 0, 0);
        pShadowEnc->setVertexBytes(&smallModelMatrix, sizeof(smallModelMatrix), 1);
        pShadowEnc->setVertexBytes(&lightViewProjectionMatrix, sizeof(lightViewProjectionMatrix), 2);
        pShadowEnc->drawIndexedPrimitives(typeTriangle,
                                          mesh->indexCount,
                                          MTL::IndexTypeUInt32,
                                          mesh->indexBuffer,
                                          0);
    }
    pShadowEnc->endEncoding();

    
    MTL::RenderPassDescriptor *pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder *pEnc = pCmd->renderCommandEncoder(pRpd);

    pEnc->setFrontFacingWinding(MTL::WindingCounterClockwise);
    pEnc->setCullMode(MTL::CullModeBack);
    
    pEnc->setRenderPipelineState(metalRenderPSO);
    pEnc->setDepthStencilState(depthStencilState);

    for (Mesh *mesh : model->meshes)
    {
        pEnc->setVertexBuffer(mesh->vertexBuffer, 0, 0);
        pEnc->setVertexBytes(&modelMatrix, sizeof(modelMatrix), 1);
        pEnc->setVertexBytes(&viewMatrix, sizeof(viewMatrix), 2); 
        pEnc->setVertexBytes(&perspectiveMatrix, sizeof(perspectiveMatrix), 3);
        pEnc->setVertexBytes(&lightViewProjectionMatrix, sizeof(lightViewProjectionMatrix), 4);
        pEnc->setFragmentBytes(&lightColor, sizeof(lightColor), 0);
        pEnc->setFragmentBytes(&lightPosition, sizeof(lightPosition), 1);
        pEnc->setFragmentBytes(&cameraPosition, sizeof(cameraPosition), 2);
        pEnc->setFragmentTexture(model->textures->textureArray, 3);
        pEnc->setFragmentTexture(shadowTexture, 4); 
        pEnc->setFragmentBuffer(model->textures->textureInfosBuffer, 0, 4);
        pEnc->setFragmentBytes(&modelMatrix, sizeof(modelMatrix), 5);

        pEnc->drawIndexedPrimitives(typeTriangle,
                                    mesh->indexCount,
                                    MTL::IndexTypeUInt32,
                                    mesh->indexBuffer,
                                    0);
    }

    for (Mesh *mesh : model->meshes)
    {
        pEnc->setVertexBuffer(mesh->vertexBuffer, 0, 0);
        pEnc->setVertexBytes(&smallModelMatrix, sizeof(smallModelMatrix), 1);
        pEnc->setVertexBytes(&viewMatrix, sizeof(viewMatrix), 2); 
        pEnc->setVertexBytes(&perspectiveMatrix, sizeof(perspectiveMatrix), 3);
        pEnc->setVertexBytes(&lightViewProjectionMatrix, sizeof(lightViewProjectionMatrix), 4);
        pEnc->setFragmentBytes(&lightColor, sizeof(lightColor), 0);
        pEnc->setFragmentBytes(&lightPosition, sizeof(lightPosition), 1);
        pEnc->setFragmentBytes(&cameraPosition, sizeof(cameraPosition), 2);
        pEnc->setFragmentTexture(model->textures->textureArray, 3);
        pEnc->setFragmentTexture(shadowTexture, 4);
        pEnc->setFragmentBuffer(model->textures->textureInfosBuffer, 0, 4);
        pEnc->setFragmentBytes(&smallModelMatrix, sizeof(smallModelMatrix), 5);

        pEnc->drawIndexedPrimitives(typeTriangle,
                                    mesh->indexCount,
                                    MTL::IndexTypeUInt32,
                                    mesh->indexBuffer,
                                    0);
    }

    
    matrix_float4x4 lightCubeScaleMatrix = matrix4x4_scale(0.3f, 0.3f, 0.3f);
    matrix_float4x4 translationMatrix = matrix4x4_translation(lightPosition[0],
                                                              lightPosition[1],
                                                              lightPosition[2]);
    matrix_float4x4 lightModelMatrix = matrix_multiply(translationMatrix, lightCubeScaleMatrix);

    pEnc->setRenderPipelineState(metalLightSourceRenderPSO);
    pEnc->setVertexBuffer(lightVertexBuffer, 0, 0);
    pEnc->setVertexBytes(&lightModelMatrix, sizeof(lightModelMatrix), 1);   
    pEnc->setVertexBytes(&perspectiveMatrix, sizeof(perspectiveMatrix), 2); 
    pEnc->setFragmentBytes(&lightColor, sizeof(lightColor), 0);
    pEnc->drawPrimitives(typeTriangle, (NS::UInteger)0, (NS::UInteger)(6 * 6));

    pEnc->endEncoding();
    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();
    pPool->release();
}

void Renderer::loadMeshes()
{
    std::string modelPath = "build/assets/gift_bag/gift_bag.dae";
    model = new Model(modelPath, _pDevice);

    VertexData lightSource[] = {
        
        {{0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        
        {{0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        
        {{-0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        
        {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
    };

    lightVertexBuffer = _pDevice->newBuffer(&lightSource, sizeof(lightSource),
                                            MTL::ResourceStorageModeShared);
}

void Renderer::createRenderPipeline()
{
    using NS::StringEncoding::UTF8StringEncoding;

    Shader sh;
    NS::Error *pError = nullptr;
    MTL::Library *pLibrary = _pDevice->newLibrary(
        NS::String::string(sh.GetShader("shaders/square.metal"), UTF8StringEncoding),
        nullptr, &pError);
    if (!pLibrary)
    {
        __builtin_printf("shaders.metal error: %s\n",
                         pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function *vertexShader = pLibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    MTL::Function *fragmentShader = pLibrary->newFunction(NS::String::string("TexturefragmentShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor *rpd = MTL::RenderPipelineDescriptor::alloc()->init();
    rpd->setVertexFunction(vertexShader);
    rpd->setFragmentFunction(fragmentShader);
    rpd->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    rpd->setSampleCount(4);
    rpd->setLabel(NS::String::string("Model Render Pipeline", NS::ASCIIStringEncoding));
    rpd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    NS::Error *error = nullptr;
    metalRenderPSO = _pDevice->newRenderPipelineState(rpd, &error);
    if (!metalRenderPSO)
    {
        __builtin_printf("PSO error: %s\n", error->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::DepthStencilDescriptor *dsd = MTL::DepthStencilDescriptor::alloc()->init();
    dsd->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    dsd->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(dsd);

    MTL::TextureDescriptor *shadowTextureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float, 1024, 1024, false);
    shadowTextureDescriptor->setStorageMode(MTL::StorageModePrivate);
    shadowTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    shadowTexture = _pDevice->newTexture(shadowTextureDescriptor);

    shadowRenderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    shadowRenderPassDescriptor->setDepthAttachment(MTL::RenderPassDepthAttachmentDescriptor::alloc()->init());
    shadowRenderPassDescriptor->depthAttachment()->setTexture(shadowTexture);
    shadowRenderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    shadowRenderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionStore);
    shadowRenderPassDescriptor->depthAttachment()->setClearDepth(1.0f);

    MTL::Function *shadowVertexShader = pLibrary->newFunction(NS::String::string("vertex_zOnly", NS::ASCIIStringEncoding));
    rpd = MTL::RenderPipelineDescriptor::alloc()->init();
    rpd->setVertexFunction(shadowVertexShader);
    rpd->setDepthAttachmentPixelFormat(shadowTexture->pixelFormat());
    shadowPipelineState = _pDevice->newRenderPipelineState(rpd, &error);
    if (!shadowPipelineState)
    {
        __builtin_printf("Shadow PSO error: %s\n", error->localizedDescription()->utf8String());
        assert(false);
    }

    dsd->release();
    rpd->release();
    vertexShader->release();
    fragmentShader->release();
    shadowVertexShader->release();
    pLibrary->release();
}

void Renderer::createLightSourceRenderPipeline()
{
    using NS::StringEncoding::UTF8StringEncoding;

    Shader sh;
    NS::Error *pError = nullptr;
    MTL::Library *pLibrary = _pDevice->newLibrary(
        NS::String::string(sh.GetShader("shaders/light.metal"), UTF8StringEncoding),
        nullptr, &pError);
    if (!pLibrary)
    {
        __builtin_printf("light.metal error: %s\n",
                         pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function *vertexShader = pLibrary->newFunction(NS::String::string("lightVertexShader", NS::ASCIIStringEncoding));
    MTL::Function *fragmentShader = pLibrary->newFunction(NS::String::string("lightFragmentShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor *rpd = MTL::RenderPipelineDescriptor::alloc()->init();
    rpd->setVertexFunction(vertexShader);
    rpd->setFragmentFunction(fragmentShader);
    rpd->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    rpd->setSampleCount(4);
    rpd->setLabel(NS::String::string("Light Source Render Pipeline", NS::ASCIIStringEncoding));
    rpd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    NS::Error *error = nullptr;
    metalLightSourceRenderPSO = _pDevice->newRenderPipelineState(rpd, &error);
    if (!metalLightSourceRenderPSO)
    {
        __builtin_printf("Light PSO error: %s\n", error->localizedDescription()->utf8String());
        assert(false);
    }

    rpd->release();
    vertexShader->release();
    fragmentShader->release();
    pLibrary->release();
}