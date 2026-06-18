#include "Renderer.hpp"
#include "Shader.h"


Renderer::Renderer( MTL::Device* pDevice )
: _pDevice( pDevice->retain() )
{
    _pCommandQueue = _pDevice->newCommandQueue();
    buildShaders();
    buildBuffers();
    loadMeshes();
}

Renderer::~Renderer()
{
    _pVertexPositionsBuffer->release();
    _pVertexColorsBuffer->release();
     metalRenderPSO->release();
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::buildShaders()
{
    using NS::StringEncoding::UTF8StringEncoding;

    Shader sh;
    const char * shadersrc = sh.GetShader("shaders/shaders.metal");

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = _pDevice->newLibrary( NS::String::string(shadersrc, UTF8StringEncoding), nullptr, &pError );
    if ( !pLibrary )
    {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }

    MTL::Function* pVertexFn = pLibrary->newFunction( NS::String::string("vertexMain", UTF8StringEncoding) );
    MTL::Function* pFragFn = pLibrary->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );

    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction( pVertexFn );
    pDesc->setFragmentFunction( pFragFn );
    pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );

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

void Renderer::buildBuffers()
{
    const size_t NumVertices = 3;

    simd::float3 positions[NumVertices] ={
        {-0.5f, -0.5f, 0.0f},
        { 0.5f, -0.5f, 0.0f},
        { 0.0f,  0.5f, 0.0f}
    };

    simd::float3 colors[NumVertices] =
    {
        {  1.0, 0.0f, 0.0f },
        {  0.0f, 1.0, 0.0f },
        {  0.0f, 0.0f, 1.0 }
    };

    const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
    const size_t colorDataSize = NumVertices * sizeof( simd::float3 );

    MTL::Buffer* pVertexPositionsBuffer = _pDevice->newBuffer( positionsDataSize, MTL::ResourceStorageModeShared );
    MTL::Buffer* pVertexColorsBuffer = _pDevice->newBuffer( colorDataSize, MTL::ResourceStorageModeShared );

    _pVertexPositionsBuffer = pVertexPositionsBuffer;
    _pVertexColorsBuffer = pVertexColorsBuffer;

    memcpy( _pVertexPositionsBuffer->contents(), positions, positionsDataSize );
    memcpy( _pVertexColorsBuffer->contents(), colors, colorDataSize );
}

void Renderer::draw( MTK::View* pView )
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

    pEnc->setRenderPipelineState( metalRenderPSO );
    pEnc->setVertexBuffer( _pVertexPositionsBuffer, 0, 0 );
    pEnc->setVertexBuffer( _pVertexColorsBuffer, 0, 1 );
    pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );

    pEnc->endEncoding();
    pCmd->presentDrawable( pView->currentDrawable() );
    pCmd->commit();
    pPool->release();
}



void Renderer::loadMeshes() {
    mesh = new Mesh("assets/box.obj", _pDevice);
    VertexData lightSource[] = {
        // Front face               // Normals
         {{ 0.5, -0.5, -0.5, 1.0f}, {0.0, 0.0,-1.0, 1.0}},// bottom-right 2
         {{ 0.5,  0.5, -0.5, 1.0f}, {0.0, 0.0,-1.0, 1.0}},// top-right    3
         {{-0.5,  0.5, -0.5, 1.0f}, {0.0, 0.0,-1.0, 1.0}},// top-left     1
         {{ 0.5, -0.5, -0.5, 1.0f}, {0.0, 0.0,-1.0, 1.0}},// bottom-right 2
         {{-0.5,  0.5, -0.5, 1.0f}, {0.0, 0.0,-1.0, 1.0}},// top-left     1
         {{-0.5, -0.5, -0.5, 1.0f}, {0.0, 0.0,-1.0, 1.0}},// bottom-left  0
        // Right face
         {{ 0.5, -0.5,  0.5, 1.0f}, {1.0, 0.0, 0.0, 1.0}}, // bottom-right 6
         {{ 0.5,  0.5,  0.5, 1.0f}, {1.0, 0.0, 0.0, 1.0}}, // top-right    7
         {{ 0.5,  0.5, -0.5, 1.0f}, {1.0, 0.0, 0.0, 1.0}}, // top-right    3
         {{ 0.5, -0.5,  0.5, 1.0f}, {1.0, 0.0, 0.0, 1.0}}, // bottom-right 6
         {{ 0.5,  0.5, -0.5, 1.0f}, {1.0, 0.0, 0.0, 1.0}}, // top-right    3
         {{ 0.5, -0.5, -0.5, 1.0f}, {1.0, 0.0, 0.0, 1.0}}, // bottom-right 2
        // Back face
         {{-0.5, -0.5,  0.5, 1.0f}, {0.0, 0.0, 1.0, 1.0}}, // bottom-left  4
         {{-0.5,  0.5,  0.5, 1.0f}, {0.0, 0.0, 1.0, 1.0}}, // top-left     5
         {{ 0.5,  0.5,  0.5, 1.0f}, {0.0, 0.0, 1.0, 1.0}}, // top-right    7
         {{-0.5, -0.5,  0.5, 1.0f}, {0.0, 0.0, 1.0, 1.0}}, // bottom-left  4
         {{ 0.5,  0.5,  0.5, 1.0f}, {0.0, 0.0, 1.0, 1.0}}, // top-right    7
         {{ 0.5, -0.5,  0.5, 1.0f}, {0.0, 0.0, 1.0, 1.0}}, // bottom-right 6
        // Left face
         {{-0.5, -0.5, -0.5, 1.0f}, {-1.0, 0.0, 0.0, 1.0}}, // bottom-left  0
         {{-0.5,  0.5, -0.5, 1.0f}, {-1.0, 0.0, 0.0, 1.0}}, // top-left     1
         {{-0.5,  0.5,  0.5, 1.0f}, {-1.0, 0.0, 0.0, 1.0}}, // top-left     5
         {{-0.5, -0.5, -0.5, 1.0f}, {-1.0, 0.0, 0.0, 1.0}}, // bottom-left  0
         {{-0.5,  0.5,  0.5, 1.0f}, {-1.0, 0.0, 0.0, 1.0}}, // top-left     5
         {{-0.5, -0.5,  0.5, 1.0f}, {-1.0, 0.0, 0.0, 1.0}}, // bottom-left  4
        // Top face
         {{-0.5,  0.5,  0.5, 1.0f}, {0.0, 1.0, 0.0, 1.0}}, // top-left     5
         {{-0.5,  0.5, -0.5, 1.0f}, {0.0, 1.0, 0.0, 1.0}}, // top-left     1
         {{ 0.5,  0.5, -0.5, 1.0f}, {0.0, 1.0, 0.0, 1.0}}, // top-right    3
         {{-0.5,  0.5,  0.5, 1.0f}, {0.0, 1.0, 0.0, 1.0}}, // top-left     5
         {{ 0.5,  0.5, -0.5, 1.0f}, {0.0, 1.0, 0.0, 1.0}}, // top-right    3
         {{ 0.5,  0.5,  0.5, 1.0f}, {0.0, 1.0, 0.0, 1.0}}, // top-right    7
        // Bottom face
         {{-0.5, -0.5, -0.5, 1.0f}, {0.0, -1.0, 0.0, 1.0}}, // bottom-left  0
         {{-0.5, -0.5,  0.5, 1.0f}, {0.0, -1.0, 0.0, 1.0}}, // bottom-left  4
         {{ 0.5, -0.5,  0.5, 1.0f}, {0.0, -1.0, 0.0, 1.0}}, // bottom-right 6
         {{-0.5, -0.5, -0.5, 1.0f}, {0.0, -1.0, 0.0, 1.0}}, // bottom-left  0
         {{ 0.5, -0.5,  0.5, 1.0f}, {0.0, -1.0, 0.0, 1.0}}, // bottom-right 6
         {{ 0.5, -0.5, -0.5, 1.0f}, {0.0, -1.0, 0.0, 1.0}}  // bottom-right 2
    };
    
    lightVertexBuffer = _pDevice->newBuffer(&lightSource, sizeof(lightSource), MTL::ResourceStorageModeShared);
}


void Renderer::createRenderPipeline() {

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
    renderPipelineDescriptor->setSampleCount(4);
    renderPipelineDescriptor->setLabel(NS::String::string("Model Render Pipeline", NS::ASCIIStringEncoding));
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    renderPipelineDescriptor->setTessellationOutputWindingOrder(MTL::WindingCounterClockwise);
    
    NS::Error* error;
    metalRenderPSO = _pDevice->newRenderPipelineState(renderPipelineDescriptor, &error);
    
    if (metalRenderPSO == nil) {
        std::exit(0);
    }
    
    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(depthStencilDescriptor);
    
    depthStencilDescriptor->release();
    renderPipelineDescriptor->release();
    vertexShader->release();
    fragmentShader->release();
}

void Renderer::createLightSourceRenderPipeline() {
    MTL::Function* vertexShader = metalDefaultLibrary->newFunction(NS::String::string("lightVertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function* fragmentShader = metalDefaultLibrary->newFunction(NS::String::string("lightFragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);
    
    MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer.pixelFormat;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->setSampleCount(4);
    renderPipelineDescriptor->setLabel(NS::String::string("Light Source Render Pipeline", NS::ASCIIStringEncoding));
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    renderPipelineDescriptor->setTessellationOutputWindingOrder(MTL::WindingCounterClockwise);
    
    NS::Error* error;
    metalLightSourceRenderPSO = _pDevice->newRenderPipelineState(renderPipelineDescriptor, &error);
    
    renderPipelineDescriptor->release();
}