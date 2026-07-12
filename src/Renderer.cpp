#include "Renderer.hpp"
#include "VertexData.hpp"
#include "Shader.h"

Renderer::Renderer(MTL::Device *pDevice)
    : _pDevice(pDevice->retain())
{
    _pCommandQueue = _pDevice->newCommandQueue();
    createDefaultLibrary(pDevice);
    buildShaders();
    CreateCube();
}

Renderer::~Renderer()
{
    planeVertexBuffer->release();
    delete D_Texture;
    delete N_Texture;
    _pPSO->release();
    _renderToTexturePipelineState->release();
    _renderTexture->release();
    _offscreenDepthTexture->release();
    _renderToTextureRenderPassDescriptor->release();
    depthStencilState->release();
    UniformBuffer->release();
    transformationBuffer->release();
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::createDefaultLibrary(MTL::Device *pDevice)
{
    using NS::StringEncoding::UTF8StringEncoding;
    Shader sh;
    const char *shadersrc = sh.GetShader("shaders/square.metal");
    assert(shadersrc);
    NS::Error *pError = nullptr;
    metallibrary = pDevice->newLibrary(
        NS::String::string(shadersrc, UTF8StringEncoding), nullptr, &pError);
    if (!metallibrary)
    {
        __builtin_printf("Compile FAILED: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }
}

void Renderer::CreateCube()
{
    glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
    glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
    glm::vec3 pos3(1.0f, -1.0f, 0.0f);
    glm::vec3 pos4(1.0f, 1.0f, 0.0f);

    glm::vec2 uv1(0.0f, 1.0f);
    glm::vec2 uv2(0.0f, 0.0f);
    glm::vec2 uv3(1.0f, 0.0f);
    glm::vec2 uv4(1.0f, 1.0f);

    glm::vec3 nm(0.0f, 0.0f, 1.0f);

    glm::vec3 tangent1, bitangent1;
    glm::vec3 tangent2, bitangent2;

    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    edge1 = pos3 - pos1;
    edge2 = pos4 - pos1;
    deltaUV1 = uv3 - uv1;
    deltaUV2 = uv4 - uv1;

    f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    std::vector<NVertexData> vertexData;

    vertexData.emplace_back(NVertexData{{pos1.x, pos1.y, pos1.z}, {nm.x, nm.y, nm.z}, {uv1.x, uv1.y}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z}});
    vertexData.emplace_back(NVertexData{{pos2.x, pos2.y, pos2.z}, {nm.x, nm.y, nm.z}, {uv2.x, uv2.y}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z}});
    vertexData.emplace_back(NVertexData{{pos3.x, pos3.y, pos3.z}, {nm.x, nm.y, nm.z}, {uv3.x, uv3.y}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z}});
    vertexData.emplace_back(NVertexData{{pos1.x, pos1.y, pos1.z}, {nm.x, nm.y, nm.z}, {uv1.x, uv1.y}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z}});
    vertexData.emplace_back(NVertexData{{pos3.x, pos3.y, pos3.z}, {nm.x, nm.y, nm.z}, {uv3.x, uv3.y}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z}});
    vertexData.emplace_back(NVertexData{{pos4.x, pos4.y, pos4.z}, {nm.x, nm.y, nm.z}, {uv4.x, uv4.y}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z}});

    planeVertexBuffer = _pDevice->newBuffer(
        vertexData.data(),
        sizeof(NVertexData) * vertexData.size(),
        MTL::ResourceStorageModeShared);

    D_Texture = new Texture("assets/diffuse.png", _pDevice);
    N_Texture = new Texture("assets/normal.png", _pDevice);
}

void Renderer::buildShaders()
{
    NS::Error *pError = nullptr;

    
    MTL::TextureDescriptor *colorDesc = MTL::TextureDescriptor::alloc()->init();
    colorDesc->setPixelFormat(MTL::PixelFormatRGBA16Float);   
    colorDesc->setWidth(512 * 4);
    colorDesc->setHeight(512 * 4);
    colorDesc->setStorageMode(MTL::StorageModePrivate);
    colorDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    _renderTexture = _pDevice->newTexture(colorDesc);
    colorDesc->release();

    
    MTL::TextureDescriptor *depthDesc = MTL::TextureDescriptor::alloc()->init();
    depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthDesc->setWidth(512 * 4);
    depthDesc->setHeight(512 * 4);
    depthDesc->setStorageMode(MTL::StorageModePrivate);
    depthDesc->setUsage(MTL::TextureUsageRenderTarget);
    _offscreenDepthTexture = _pDevice->newTexture(depthDesc);
    depthDesc->release();

    _renderToTextureRenderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    _renderToTextureRenderPassDescriptor->colorAttachments()->object(0)->setTexture(_renderTexture);
    _renderToTextureRenderPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
    _renderToTextureRenderPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    _renderToTextureRenderPassDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(0.0, 0.0, 0.0, 1.0));
    _renderToTextureRenderPassDescriptor->depthAttachment()->setTexture(_offscreenDepthTexture);
    _renderToTextureRenderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    _renderToTextureRenderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    _renderToTextureRenderPassDescriptor->depthAttachment()->setClearDepth(1.0);

    
    MTL::Function *vertexFn = metallibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    assert(vertexFn);
    MTL::Function *fragmentFn = metallibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentFn);

    MTL::RenderPipelineDescriptor *pDesc1 = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc1->setVertexFunction(vertexFn);
    pDesc1->setFragmentFunction(fragmentFn);
    pDesc1->colorAttachments()->object(0)->setPixelFormat(_renderTexture->pixelFormat());
    pDesc1->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    _pPSO = _pDevice->newRenderPipelineState(pDesc1, &pError);
    if (!_pPSO)
        __builtin_printf("PSO1 FAILED: %s\n", pError->localizedDescription()->utf8String());
    assert(_pPSO);
    vertexFn->release();
    fragmentFn->release();
    pDesc1->release();

    MTL::DepthStencilDescriptor *depthStencilDesc = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthStencilDesc->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(depthStencilDesc);
    depthStencilDesc->release();

    UniformBuffer = _pDevice->newBuffer(sizeof(N_Uniforms), MTL::ResourceStorageModeShared);
    transformationBuffer = _pDevice->newBuffer(sizeof(N_MVP), MTL::ResourceStorageModeShared);

    
    MTL::Function *vertexRPFn = metallibrary->newFunction(NS::String::string("vertexRenderPass", NS::ASCIIStringEncoding));
    assert(vertexRPFn);
    MTL::Function *fragmentRPFn = metallibrary->newFunction(NS::String::string("fragmentRenderPass", NS::ASCIIStringEncoding));
    assert(fragmentRPFn);

    MTL::RenderPipelineDescriptor *pDesc2 = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc2->setVertexFunction(vertexRPFn);
    pDesc2->setFragmentFunction(fragmentRPFn);
    pDesc2->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    _renderToTexturePipelineState = _pDevice->newRenderPipelineState(pDesc2, &pError);
    if (!_renderToTexturePipelineState)
        __builtin_printf("PSO2 FAILED: %s\n", pError->localizedDescription()->utf8String());
    assert(_renderToTexturePipelineState);
    vertexRPFn->release();
    fragmentRPFn->release();
    pDesc2->release();
}

void Renderer::draw(MTK::View *pView)
{
    NS::AutoreleasePool *pPool = NS::AutoreleasePool::alloc()->init();
    MTL::CommandBuffer *pCmd = _pCommandQueue->commandBuffer();

    
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

        static float accumulatedDegrees = 0.0f;
        const float rotationSpeedDegreesPerSecond = 45.0f;
        accumulatedDegrees += rotationSpeedDegreesPerSecond * Time::DeltaTime;
        if (accumulatedDegrees >= 360.0f)
            accumulatedDegrees -= 360.0f;

        float angleInRadians = accumulatedDegrees * (M_PI / 180.0f);
        model = glm::rotate(model, angleInRadians, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 viewMatrix = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));

        auto drawableSize = pView->drawableSize();
        float aspectRatio = (float)drawableSize.width / (float)drawableSize.height;
        glm::mat4 perspectiveMatrix = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 100.0f);

        N_MVP mvp1;
        mvp1.M = matrix_float4x4({
            simd::float4{model[0][0], model[0][1], model[0][2], model[0][3]},
            simd::float4{model[1][0], model[1][1], model[1][2], model[1][3]},
            simd::float4{model[2][0], model[2][1], model[2][2], model[2][3]},
            simd::float4{model[3][0], model[3][1], model[3][2], model[3][3]},
        });
        mvp1.V = matrix_float4x4({
            simd::float4{viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2], viewMatrix[0][3]},
            simd::float4{viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2], viewMatrix[1][3]},
            simd::float4{viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2], viewMatrix[2][3]},
            simd::float4{viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2], viewMatrix[3][3]},
        });
        mvp1.P = matrix_float4x4({
            simd::float4{perspectiveMatrix[0][0], perspectiveMatrix[0][1], perspectiveMatrix[0][2], perspectiveMatrix[0][3]},
            simd::float4{perspectiveMatrix[1][0], perspectiveMatrix[1][1], perspectiveMatrix[1][2], perspectiveMatrix[1][3]},
            simd::float4{perspectiveMatrix[2][0], perspectiveMatrix[2][1], perspectiveMatrix[2][2], perspectiveMatrix[2][3]},
            simd::float4{perspectiveMatrix[3][0], perspectiveMatrix[3][1], perspectiveMatrix[3][2], perspectiveMatrix[3][3]},
        });
        memcpy(transformationBuffer->contents(), &mvp1, sizeof(N_MVP));

        glm::mat4 cameraToWorld = glm::inverse(viewMatrix);
        glm::vec3 cameraPos = glm::vec3(cameraToWorld[3]);
        float3 mslVec1 = *reinterpret_cast<float3 *>(&cameraPos);

        glm::mat3 normalMatrix = glm::transpose(inverse(glm::mat3(model)));
        N_Uniforms uniforms;
        uniforms.lightPos = float3{1.5f, 1.5f, 2.0f};
        uniforms.viewPos = mslVec1;
        uniforms.normalMatrix = float3x3{
            simd::float3{normalMatrix[0][0], normalMatrix[0][1], normalMatrix[0][2]},
            simd::float3{normalMatrix[1][0], normalMatrix[1][1], normalMatrix[1][2]},
            simd::float3{normalMatrix[2][0], normalMatrix[2][1], normalMatrix[2][2]}};
        memcpy(UniformBuffer->contents(), &uniforms, sizeof(N_Uniforms));

        MTL::RenderCommandEncoder *pEnc1 = pCmd->renderCommandEncoder(_renderToTextureRenderPassDescriptor);
        pEnc1->setRenderPipelineState(_pPSO);
        pEnc1->setDepthStencilState(depthStencilState);
        pEnc1->setVertexBuffer(planeVertexBuffer, 0, 0);
        pEnc1->setVertexBuffer(UniformBuffer, 0, 1);
        pEnc1->setVertexBuffer(transformationBuffer, 0, 2);
        pEnc1->setFragmentTexture(D_Texture->texture, 0);
        pEnc1->setFragmentTexture(N_Texture->texture, 1);
        pEnc1->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));
        pEnc1->endEncoding();
    }

   
    {
        MTL::RenderPassDescriptor *pRpd2 = pView->currentRenderPassDescriptor();
        MTL::RenderCommandEncoder *pEnc2 = pCmd->renderCommandEncoder(pRpd2);
        pEnc2->setRenderPipelineState(_renderToTexturePipelineState);

        static const AAPLVertex quadVertices[] = {
            {{-1.0, -1.0}, {0.0, 0.0, 0.0, 1.0}, {0.0, 1.0}},
            {{1.0, -1.0}, {1.0, 0.0, 0.0, 1.0}, {1.0, 1.0}},
            {{1.0, 1.0}, {1.0, 1.0, 0.0, 1.0}, {1.0, 0.0}},
            {{1.0, 1.0}, {1.0, 1.0, 0.0, 1.0}, {1.0, 0.0}},
            {{-1.0, 1.0}, {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0}},
            {{-1.0, -1.0}, {0.0, 0.0, 0.0, 1.0}, {0.0, 1.0}},
        };
        pEnc2->setVertexBytes(quadVertices, sizeof(quadVertices), 0);
        pEnc2->setFragmentTexture(_renderTexture, 0);
        pEnc2->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));
        pEnc2->endEncoding();
    }

    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();
    pPool->release();
}