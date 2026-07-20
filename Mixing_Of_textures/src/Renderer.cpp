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

    __builtin_printf("Step 4: createSquare\n");
    createSquare();

    __builtin_printf("Step 5: constructor done\n");
}

Renderer::~Renderer()
{
    squareVertexBuffer->release();

    _pPSO->release();
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::createDefaultLibrary(MTL::Device* pDevice) {
    using NS::StringEncoding::UTF8StringEncoding;

    // Step 1: Check working directory
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    __builtin_printf("CWD: %s\n", cwd);

    // Step 2: Check shader source loaded
    Shader sh;
    const char* shadersrc = sh.GetShader("shaders/square.metal");
    if (!shadersrc) {
        __builtin_printf("ERROR: GetShader returned null — file not found\n");
        assert(false);
    }
    __builtin_printf("Shader source loaded, first 100 chars:\n%.100s\n", shadersrc);

    // Step 3: Compile
    NS::Error* pError = nullptr;
    metallibrary = pDevice->newLibrary(
        NS::String::string(shadersrc, UTF8StringEncoding), nullptr, &pError
    );

    if (!metallibrary) {
        __builtin_printf("Compile FAILED: %s\n",
            pError->localizedDescription()->utf8String());
        assert(false);
    }
    __builtin_printf("Library compiled OK\n");

    // Step 4: List all functions in the library
    NS::Array* fnames = metallibrary->functionNames();
    __builtin_printf("Function count: %lu\n", fnames->count());
    for (uint32_t i = 0; i < fnames->count(); ++i) {
        __builtin_printf("  fn: %s\n", ((NS::String*)fnames->object(i))->utf8String());
    }
}
void Renderer::buildShaders()
{
    using NS::StringEncoding::UTF8StringEncoding;

    NS::Error* pError = nullptr;

    MTL::Function* vertexShader = metallibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    if (!vertexShader)
    {
        __builtin_printf("ERROR: vertex function 'vertexShader' not found in library.\n");
        NS::Array* fnames = metallibrary->functionNames();
        if (fnames)
        {
            for (uint32_t i = 0; i < fnames->count(); ++i)
            {
                NS::String* n = (NS::String*)fnames->object(i);
                __builtin_printf(" available: %s\n", n->utf8String());
            }
        }
        assert(false);
    }
    MTL::Function* fragmentShader = metallibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    if (!fragmentShader)
    {
        __builtin_printf("ERROR: fragment function 'fragmentShader' not found in library.\n");
        NS::Array* fnames = metallibrary->functionNames();
        if (fnames)
        {
            for (uint32_t i = 0; i < fnames->count(); ++i)
            {
                NS::String* n = (NS::String*)fnames->object(i);
                __builtin_printf(" available: %s\n", n->utf8String());
            }
        }
        assert(false);
    }

    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction( vertexShader );
    pDesc->setFragmentFunction( fragmentShader);
    pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );


    _pPSO = _pDevice->newRenderPipelineState( pDesc, &pError );
    if ( !_pPSO )
    {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }

    UniformBuffer =_pDevice->newBuffer(sizeof(Uniforms),MTL::ResourceStorageModeShared);
    Uniform1Buffer =_pDevice->newBuffer(sizeof(Uniforms),MTL::ResourceStorageModeShared);

    fragmentShader->release();
    vertexShader->release();
    pDesc->release();
    
}

void Renderer::createSquare() {

    VertexData squareVertices[] {
        {{-0.5, -0.5,  0.5, 1.0f}, {0.0f, 0.0f}},
        {{-0.5,  0.5,  0.5, 1.0f}, {0.0f, 1.0f}},
        {{ 0.5,  0.5,  0.5, 1.0f}, {1.0f, 1.0f}},
        {{-0.5, -0.5,  0.5, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5,  0.5,  0.5, 1.0f}, {1.0f, 1.0f}},
        {{ 0.5, -0.5,  0.5, 1.0f}, {1.0f, 0.0f}}
    };
    
    squareVertexBuffer = _pDevice->newBuffer(&squareVertices, sizeof(squareVertices), MTL::ResourceStorageModeShared);
     __builtin_printf("squareVertexBuffer: %p\n", squareVertexBuffer);

    grassTexture = new Texture("assets/container.jpg", _pDevice);
    smileyTexture = new Texture("assets/awesomeface.png",_pDevice);

    // textures.emplace_back(new Texture("assets/captain/Attack 2 01.png", _pDevice));
    // textures.emplace_back(new Texture("assets/captain/Attack 2 02.png", _pDevice));
    // textures.emplace_back(new Texture("assets/captain/Attack 2 03.png", _pDevice));
}
void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    Uniforms uniforms;
    Uniforms1 uniforms1;
    uniforms.time = {0.1f,0.3f};
    uniforms1.intAsBool = 1;

    memcpy(UniformBuffer->contents(),&uniforms,sizeof(Uniforms));
    memcpy(Uniform1Buffer->contents(),&uniforms1,sizeof(Uniforms1));

    
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

    __builtin_printf("draw OK — encoder created\n");


    pEnc->setVertexBuffer(UniformBuffer,0,1);   // index 1
    pEnc->setVertexBuffer(Uniform1Buffer,0,2);   // index 1


    pEnc->setRenderPipelineState(_pPSO);

    pEnc->setVertexBuffer(squareVertexBuffer, 0, 0);

    
    pEnc->setFragmentTexture(grassTexture->texture,0);
    pEnc->setFragmentTexture(smileyTexture->texture,1);


    

    // std::this_thread::sleep_for(std::chrono::milliseconds(300));
  


    pEnc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));

    pEnc->endEncoding();
    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();

    pPool->release();
}