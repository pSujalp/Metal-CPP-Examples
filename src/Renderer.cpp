#include "Renderer.hpp"
#include "Shader.h"


Renderer::Renderer( MTL::Device* pDevice )
: _pDevice( pDevice->retain() )
{
    _pCommandQueue = _pDevice->newCommandQueue();
    buildShaders();
    buildBuffers();
}

Renderer::~Renderer()
{
    _pVertexPositionsBuffer->release();
    _pVertexColorsBuffer->release();
    _pPSO->release();
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

    MTL::Texture* readTexture = pView->currentDrawable()->texture();
    NS::UInteger width = readTexture->width();
    NS::UInteger height = readTexture->height();
    NS::UInteger bytesPerPixel = 4;
    NS::UInteger bytesPerRow = width * bytesPerPixel;
    NS::UInteger bufferSize = bytesPerRow * height;

    MTL::Buffer* pPixelBuffer = _pDevice->newBuffer( bufferSize, MTL::ResourceStorageModeShared );

    MTL::BlitCommandEncoder* pBlitEncoder = pCmd->blitCommandEncoder();
    pBlitEncoder->copyFromTexture(
        readTexture,
        0, 0,
        MTL::Origin(0, 0, 0),
        MTL::Size(width, height, 1),
        pPixelBuffer,
        0,
        bytesPerRow,
        bufferSize
    );
    pBlitEncoder->endEncoding();   

  
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

    pEnc->setRenderPipelineState( _pPSO );
    pEnc->setVertexBuffer( _pVertexPositionsBuffer, 0, 0 );
    pEnc->setVertexBuffer( _pVertexColorsBuffer, 0, 1 );
    pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );
    pEnc->endEncoding();

    pCmd->presentDrawable( pView->currentDrawable() );
    pCmd->commit();
    pCmd->waitUntilCompleted();


    std::vector<uint8_t> pixelData(bytesPerRow * height);
    memcpy(pixelData.data(), pPixelBuffer->contents(), pixelData.size());
    stbi_write_png("screenshot.png", width, height, 4, pixelData.data(), width * 4);



    pPixelBuffer->release(); 
    pPool->release();
}