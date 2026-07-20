#include "Renderer.hpp"


void Renderer::draw( MTK::View* pView )
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

        glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, -1.0f));
    model = glm::scale(model, glm::vec3(4.5f, 1.5f, 1.5f));

    
    static float accumulatedDegrees = 0.0f;
    const float rotationSpeedDegreesPerSecond = 45.0f;
    accumulatedDegrees += rotationSpeedDegreesPerSecond * Time::DeltaTime;
    if (accumulatedDegrees >= 360.0f)
        accumulatedDegrees -= 360.0f;

    
    float angleInRadians = accumulatedDegrees * (M_PI / 180.0f);
    model = glm::rotate(model, angleInRadians, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f,  5.0f),   
        glm::vec3(0.0f, 0.0f,  0.0f),   
        glm::vec3(0.0f, 1.0f,  0.0f)    
    );

    
    auto drawableSize = pView->drawableSize();
    float aspectRatio = (float)drawableSize.width / (float)drawableSize.height;
    float fov  = glm::radians(60.0f);   
    float nearZ = 0.1f;
    float farZ  = 100.0f;
    glm::mat4 perspectiveMatrix = glm::perspective(fov, aspectRatio, nearZ, farZ);
    glm::mat4 MVP_GLM = perspectiveMatrix * viewMatrix * model;

    MVP mvp1;
    mvp1.MVP = matrix_float4x4({
        simd::float4{ MVP_GLM[0][0], MVP_GLM[0][1], MVP_GLM[0][2], MVP_GLM[0][3] },
        simd::float4{ MVP_GLM[1][0], MVP_GLM[1][1], MVP_GLM[1][2], MVP_GLM[1][3] },
        simd::float4{ MVP_GLM[2][0], MVP_GLM[2][1], MVP_GLM[2][2], MVP_GLM[2][3] },
        simd::float4{ MVP_GLM[3][0], MVP_GLM[3][1], MVP_GLM[3][2], MVP_GLM[3][3] },
    });
    memcpy(transformationBuffer->contents(), &mvp1, sizeof(MVP));

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );


     pEnc->setDepthStencilState(depthStencilState);
    pEnc->setRenderPipelineState(_pPSO);
   

    pEnc->setVertexBuffer(cubeVertexBuffer,      0, 0);  
 
    pEnc->setVertexBuffer(transformationBuffer,  0, 1);  

    pEnc->drawPrimitives(MTL::PrimitiveTypeLineStrip, NS::UInteger(0), NS::UInteger(36));


    pEnc->endEncoding();
    pCmd->presentDrawable( pView->currentDrawable() );
    pCmd->commit();

    pPool->release();
}

Renderer::Renderer( MTL::Device* pDevice ) : _pDevice( pDevice->retain() )
{
    _pCommandQueue = _pDevice->newCommandQueue();
    makePipeline();
    buildBuffers();
}

Renderer::~Renderer()
{
    _pCommandQueue->release();
    _pDevice->release();
}


 void Renderer::makePipeline()
{
    using NS::StringEncoding::UTF8StringEncoding;

    Shader sh;

    const char* shaderSrc = sh.GetShader("shaders/shaders.metal");
    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = _pDevice->newLibrary( NS::String::string(shaderSrc, NS::UTF8StringEncoding), nullptr, &pError );
    if ( !pLibrary )
    {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }
    MTL::Function* pVertexFn = pLibrary->newFunction( NS::String::string("vertexShader", UTF8StringEncoding) );
    MTL::Function* pFragFn = pLibrary->newFunction( NS::String::string("fragmentShader", UTF8StringEncoding) );

    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction( pVertexFn );
    pDesc->setFragmentFunction( pFragFn );
    pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    MTL::DepthStencilDescriptor* depthStencilDescriptor =
        MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(depthStencilDescriptor);
    depthStencilDescriptor->release();


    _pPSO = _pDevice->newRenderPipelineState( pDesc, &pError );
    if ( !_pPSO )
    {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }
    pVertexFn->release();
    pFragFn->release();
    pDesc->release();
    pLibrary->release();
}


void Renderer::buildBuffers(){
   VertexData cubeVertices[] = {
    {{-0.5, -0.5,  0.5, 1.0}},
    {{ 0.5, -0.5,  0.5, 1.0}},
    {{ 0.5,  0.5,  0.5, 1.0}},
    {{ 0.5,  0.5,  0.5, 1.0}},
    {{-0.5,  0.5,  0.5, 1.0}},
    {{-0.5, -0.5,  0.5, 1.0}},

    {{ 0.5, -0.5, -0.5, 1.0}},
    {{-0.5, -0.5, -0.5, 1.0}},
    {{-0.5,  0.5, -0.5, 1.0}},
    {{-0.5,  0.5, -0.5, 1.0}},
    {{ 0.5,  0.5, -0.5, 1.0}},
    {{ 0.5, -0.5, -0.5, 1.0}},

    {{-0.5,  0.5,  0.5, 1.0}},
    {{ 0.5,  0.5,  0.5, 1.0}},
    {{ 0.5,  0.5, -0.5, 1.0}},
    {{ 0.5,  0.5, -0.5, 1.0}},
    {{-0.5,  0.5, -0.5, 1.0}},
    {{-0.5,  0.5,  0.5, 1.0}},

    {{-0.5, -0.5, -0.5, 1.0}},
    {{ 0.5, -0.5, -0.5, 1.0}},
    {{ 0.5, -0.5,  0.5, 1.0}},
    {{ 0.5, -0.5,  0.5, 1.0}},
    {{-0.5, -0.5,  0.5, 1.0}},
    {{-0.5, -0.5, -0.5, 1.0}},

    {{-0.5, -0.5, -0.5, 1.0}},
    {{-0.5, -0.5,  0.5, 1.0}},
    {{-0.5,  0.5,  0.5, 1.0}},
    {{-0.5,  0.5,  0.5, 1.0}},
    {{-0.5,  0.5, -0.5, 1.0}},
    {{-0.5, -0.5, -0.5, 1.0}},

    {{ 0.5, -0.5,  0.5, 1.0}},
    {{ 0.5, -0.5, -0.5, 1.0}},
    {{ 0.5,  0.5, -0.5, 1.0}},
    {{ 0.5,  0.5, -0.5, 1.0}},
    {{ 0.5,  0.5,  0.5, 1.0}},
    {{ 0.5, -0.5,  0.5, 1.0}},
};

cubeVertexBuffer = _pDevice->newBuffer(
        &cubeVertices, sizeof(cubeVertices), MTL::ResourceStorageModeShared
    );
transformationBuffer = _pDevice->newBuffer(sizeof(MVP),MTL::ResourceStorageModeShared);
}
