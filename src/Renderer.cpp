#include "Renderer.hpp"
#include "Shader.h"


Renderer::Renderer( MTL::Device* pDevice )
: _pDevice( pDevice->retain() )
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
    delete model;
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::draw( MTK::View* pView )
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

    pEnc->setFrontFacingWinding(MTL::WindingCounterClockwise);
    pEnc->setCullMode(MTL::CullModeBack);
    // pEnc->setTriangleFillMode(MTL::TriangleFillModeLines);
    pEnc->setRenderPipelineState(metalRenderPSO);
    pEnc->setDepthStencilState(depthStencilState);

    matrix_float4x4 rotationMatrix = matrix4x4_rotation(45 * (M_PI / 180.0f), 0.0, 1.0, 0.0);
    matrix_float4x4 modelMatrix    = matrix4x4_translation(0.0f, 0.0f, -15.2f) * rotationMatrix;

    // Fixed view matrix — replace eye/center when you add a camera
    matrix_float4x4 viewMatrix = matrix_look_at_right_hand(
        simd_make_float3(0.0f, 0.0f,  0.0f),   // eye
        simd_make_float3(0.0f, 0.0f, -1.0f),   // center
        simd_make_float3(0.0f, 1.0f,  0.0f)    // up
    );

    simd_float4 lightColor    = simd_make_float4(1.0f, 1.0f, 1.0f, 1.0f);
    simd_float4 lightPosition = simd_make_float4(60.0f, 0.6f, 0.0f, 1.0f);
    simd_float3 cameraPosition = simd_make_float3(0.0f, 0.0f, 0.0f);

    auto drawableSize = pView->drawableSize();
    float aspectRatio = (float)(drawableSize.width / drawableSize.height);
    float fov   = 45.0f * (M_PI / 180.0f);
    float nearZ = 0.1f;
    float farZ  = 100.0f;
    matrix_float4x4 perspectiveMatrix = matrix_perspective_right_hand(fov, aspectRatio, nearZ, farZ);

    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;

    for (Mesh* mesh : model->meshes)
    {
        pEnc->setVertexBuffer(mesh->vertexBuffer, 0, 0);
        pEnc->setVertexBytes(&modelMatrix,       sizeof(modelMatrix),       1);
        pEnc->setVertexBytes(&viewMatrix,        sizeof(viewMatrix),        2); // [[buffer(2)]]
        pEnc->setVertexBytes(&perspectiveMatrix, sizeof(perspectiveMatrix), 3);
        pEnc->setFragmentBytes(&lightColor,      sizeof(lightColor),        0);
        pEnc->setFragmentBytes(&lightPosition,   sizeof(lightPosition),     1);
        pEnc->setFragmentBytes(&cameraPosition,  sizeof(cameraPosition),    2);
        pEnc->setFragmentTexture(model->textures->textureArray, 3);
        pEnc->setFragmentBuffer(model->textures->textureInfosBuffer, 0, 4);
        pEnc->setFragmentBytes(&modelMatrix,     sizeof(modelMatrix),       5);

        pEnc->drawIndexedPrimitives(typeTriangle,
                                    mesh->indexCount,
                                    MTL::IndexTypeUInt32,
                                    mesh->indexBuffer,
                                    0);
    }

    // Light source cube
    matrix_float4x4 scaleMatrix       = matrix4x4_scale(0.3f, 0.3f, 0.3f);
    matrix_float4x4 translationMatrix = matrix4x4_translation(lightPosition[0],
                                                              lightPosition[1],
                                                              lightPosition[2]);
    matrix_float4x4 lightModelMatrix  = matrix_multiply(translationMatrix, scaleMatrix);

    pEnc->setRenderPipelineState(metalLightSourceRenderPSO);
    pEnc->setVertexBuffer(lightVertexBuffer, 0, 0);
    pEnc->setVertexBytes(&lightModelMatrix,  sizeof(lightModelMatrix),  1); // [[buffer(1)]]
    pEnc->setVertexBytes(&perspectiveMatrix, sizeof(perspectiveMatrix), 2); // [[buffer(2)]]
    pEnc->setFragmentBytes(&lightColor, sizeof(lightColor), 0);
    pEnc->drawPrimitives(typeTriangle, (NS::UInteger)0, (NS::UInteger)(6 * 6));

    pEnc->endEncoding();
    pCmd->presentDrawable( pView->currentDrawable() );
    pCmd->commit();
    pPool->release();
}


void Renderer::loadMeshes()
{
    model = new Model("assets/backpack/backpack.obj", _pDevice);

    VertexData lightSource[] = {
        // Front face
        {{ 0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f,-1.0f, 1.0f}},
        // Right face
        {{ 0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        // Back face
        {{-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        // Left face
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        // Top face
        {{-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        // Bottom face
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, -1.0f, 0.0f, 1.0f}},
    };

    lightVertexBuffer = _pDevice->newBuffer(&lightSource, sizeof(lightSource),
                                            MTL::ResourceStorageModeShared);
}


void Renderer::createRenderPipeline()
{
    using NS::StringEncoding::UTF8StringEncoding;

    Shader sh;
    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = _pDevice->newLibrary(
        NS::String::string(sh.GetShader("shaders/shaders.metal"), UTF8StringEncoding),
        nullptr, &pError);
    if (!pLibrary) {
        __builtin_printf("shaders.metal error: %s\n",
                         pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* vertexShader   = pLibrary->newFunction(NS::String::string("vertexShader",   NS::ASCIIStringEncoding));
    MTL::Function* fragmentShader = pLibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor* rpd = MTL::RenderPipelineDescriptor::alloc()->init();
    rpd->setVertexFunction(vertexShader);
    rpd->setFragmentFunction(fragmentShader);
    rpd->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    rpd->setSampleCount(4);
    rpd->setLabel(NS::String::string("Model Render Pipeline", NS::ASCIIStringEncoding));
    rpd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    NS::Error* error = nullptr;
    metalRenderPSO = _pDevice->newRenderPipelineState(rpd, &error);
    if (!metalRenderPSO) {
        __builtin_printf("PSO error: %s\n", error->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::DepthStencilDescriptor* dsd = MTL::DepthStencilDescriptor::alloc()->init();
    dsd->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    dsd->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(dsd);

    dsd->release();
    rpd->release();
    vertexShader->release();
    fragmentShader->release();
    pLibrary->release();
}


void Renderer::createLightSourceRenderPipeline()
{
    using NS::StringEncoding::UTF8StringEncoding;

    Shader sh;
    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = _pDevice->newLibrary(
        NS::String::string(sh.GetShader("shaders/light.metal"), UTF8StringEncoding),
        nullptr, &pError);
    if (!pLibrary) {
        __builtin_printf("light.metal error: %s\n",
                         pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* vertexShader   = pLibrary->newFunction(NS::String::string("lightVertexShader",   NS::ASCIIStringEncoding));
    MTL::Function* fragmentShader = pLibrary->newFunction(NS::String::string("lightFragmentShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor* rpd = MTL::RenderPipelineDescriptor::alloc()->init();
    rpd->setVertexFunction(vertexShader);
    rpd->setFragmentFunction(fragmentShader);
    rpd->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    rpd->setSampleCount(4);
    rpd->setLabel(NS::String::string("Light Source Render Pipeline", NS::ASCIIStringEncoding));
    rpd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    NS::Error* error = nullptr;
    metalLightSourceRenderPSO = _pDevice->newRenderPipelineState(rpd, &error);
    if (!metalLightSourceRenderPSO) {
        __builtin_printf("Light PSO error: %s\n", error->localizedDescription()->utf8String());
        assert(false);
    }

    rpd->release();
    vertexShader->release();
    fragmentShader->release();
    pLibrary->release();
}