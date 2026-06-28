#include "Renderer.hpp"


void Renderer::draw( MTK::View* pView )
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );
    pEnc->endEncoding();
    pCmd->presentDrawable( pView->currentDrawable() );
    pCmd->commit();

    pPool->release();
}

Renderer::Renderer( MTL::Device* pDevice ) : _pDevice( pDevice->retain() )
{
    _pCommandQueue = _pDevice->newCommandQueue();
    makePipeline();
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
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
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
}
