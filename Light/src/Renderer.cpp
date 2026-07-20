#include "Renderer.hpp"



Renderer::Renderer( MTL::Device* pDevice )
: _pDevice( pDevice->retain() )
{
    _pCommandQueue = _pDevice->newCommandQueue();
    CreateCubeAndLight();
    createLightSourceRenderPipeline();
    buildShaders();
    createBuffers();
}

Renderer::~Renderer()
{
    cubeVertexBuffer->release();
    lightVertexBuffer->release();
    _pPSO->release();
    metalRenderPSO->release();          // ← if you're using metalRenderPSO
    metalLightSourceRenderPSO->release(); // ← add this
    depthStencilState->release();        // ← add this too
    cubeTransformationBuffer->release(); // ← add this
    lightTransformationBuffer->release();// ← add this
    _pCommandQueue->release();
    _pDevice->release();
}
void Renderer::CreateCubeAndLight(){
   
    // Cube for use in a right-handed coordinate system with triangle faces
    // specified with a Counter-Clockwise winding order.
    VertexData cubeVertices[] = {
        // Front face            // Normals
        {{-0.5,-0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{ 0.5,-0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{ 0.5, 0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{ 0.5, 0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{-0.5,-0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        
        // Back face
        {{ 0.5,-0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{-0.5,-0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{-0.5, 0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{-0.5, 0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{ 0.5,-0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},

        // Top face
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.5, 0.5, 0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{-0.5, 0.5,-0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},

        // Bottom face
        {{-0.5,-0.5,-0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{ 0.5,-0.5,-0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{-0.5,-0.5, 0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{-0.5,-0.5,-0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},

        // Left face
        {{-0.5,-0.5,-0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5,-0.5, 0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5, 0.5,-0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5,-0.5,-0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},

        // Right face
        {{ 0.5,-0.5, 0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5,-0.5,-0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5, 0.5, 0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
    };
    
    cubeVertexBuffer = _pDevice->newBuffer(&cubeVertices, sizeof(cubeVertices), MTL::ResourceStorageModeShared);
    
    VertexData lightSource[] = {
        // Front face            // Normals
        {{-0.5,-0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{ 0.5,-0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{ 0.5, 0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{ 0.5, 0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        {{-0.5,-0.5, 0.5, 1.0}, {0.0, 0.0, 1.0, 1.0}},
        
        // Back face
        {{ 0.5,-0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{-0.5,-0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{-0.5, 0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{-0.5, 0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},
        {{ 0.5,-0.5,-0.5, 1.0}, {0.0, 0.0,-1.0, 1.0}},

        // Top face
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.5, 0.5, 0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{-0.5, 0.5,-0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}},

        // Bottom face
        {{-0.5,-0.5,-0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{ 0.5,-0.5,-0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{-0.5,-0.5, 0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},
        {{-0.5,-0.5,-0.5, 1.0}, {0.0,-1.0, 0.0, 1.0}},

        // Left face
        {{-0.5,-0.5,-0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5,-0.5, 0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5, 0.5, 0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5, 0.5,-0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},
        {{-0.5,-0.5,-0.5, 1.0}, {-1.0,0.0, 0.0, 1.0}},

        // Right face
        {{ 0.5,-0.5, 0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5,-0.5,-0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5, 0.5,-0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5, 0.5, 0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}},
    };
    
    lightVertexBuffer = _pDevice->newBuffer(&lightSource, sizeof(lightSource), MTL::ResourceStorageModeShared);
}

void Renderer::createBuffers() {
    cubeTransformationBuffer = _pDevice->newBuffer(sizeof(TransformationData), MTL::ResourceStorageModeShared);
    lightTransformationBuffer = _pDevice->newBuffer(sizeof(TransformationData), MTL::ResourceStorageModeShared);
}

void Renderer::buildShaders()
{
    using NS::StringEncoding::UTF8StringEncoding;

    Shader sh;
    NS::Error* pError = nullptr;

    MTL::Library* pLibrary = _pDevice->newLibrary( NS::String::string(sh.GetShader("shaders/shaders.metal"), UTF8StringEncoding), nullptr, &pError );
    
    MTL::Function* vertexShader = pLibrary->newFunction(NS::String::string("vertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function* fragmentShader = pLibrary->newFunction(NS::String::string("fragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);
    
    MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat) MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->setSampleCount(4);
    renderPipelineDescriptor->setLabel(NS::String::string("Cube Render Pipeline", NS::ASCIIStringEncoding));
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    renderPipelineDescriptor->setTessellationOutputWindingOrder(MTL::WindingClockwise);
    
    NS::Error* error;
    metalRenderPSO =_pDevice->newRenderPipelineState(renderPipelineDescriptor, &error);
    
    assert(metalRenderPSO != nil);
    
    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(depthStencilDescriptor);
    
    renderPipelineDescriptor->release();
    vertexShader->release();
    fragmentShader->release();
}

void Renderer::createLightSourceRenderPipeline() {
    using NS::StringEncoding::UTF8StringEncoding;
    Shader sh;
    NS::Error* pError = nullptr;  // ← add this

    MTL::Library* pLibrary = _pDevice->newLibrary(NS::String::string(sh.GetShader("shaders/light.metal"), UTF8StringEncoding), nullptr, &pError);
    // assert(pLibrary);            

    MTL::Function* vertexShader = pLibrary->newFunction(NS::String::string("lightVertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function* fragmentShader = pLibrary->newFunction(NS::String::string("lightFragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);

    MTL::PixelFormat pixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->setSampleCount(4);
    renderPipelineDescriptor->setLabel(NS::String::string("Light Source Render Pipeline", NS::ASCIIStringEncoding));
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    renderPipelineDescriptor->setTessellationOutputWindingOrder(MTL::WindingClockwise);

    NS::Error* error = nullptr;   // ← initialize to nullptr
    metalLightSourceRenderPSO = _pDevice->newRenderPipelineState(renderPipelineDescriptor, &error);
    assert(metalLightSourceRenderPSO);

    // ← release everything
    renderPipelineDescriptor->release();
    vertexShader->release();
    fragmentShader->release();
    pLibrary->release();
}

void Renderer::draw( MTK::View* pView )
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

    matrix_float4x4 translationMatrix = matrix4x4_translation(0.0f, -0.9f, 0.0f);

    matrix_float4x4 modelMatrix = translationMatrix;
    
    float time = rand()%(30);
    float oscillation = sin(time);  // oscillates between -1 and 1
    float zPosition = 1.5 + 1.5 * oscillation;  // maps oscillation to range [0, 3]

    matrix_float4x4 viewMatrix = matrix_look_at_right_hand(0.0f, 0.0f, 1.0f,
                                                           0.0f, 0.0f, 0.0f,
                                                           0.0f, 1.0f, 0.0f);

    CGSize drawableSize = pView->drawableSize();
    float aspectRatio = static_cast<float>(drawableSize.width / drawableSize.height);
    float fov = 90 * (M_PI / 180.0f);
    float nearZ = 0.1f;
    float farZ = 100.0f;
    
    matrix_float4x4 perspectiveMatrix = matrix_perspective_right_hand(fov, aspectRatio, nearZ, farZ);
    TransformationData transformationData = { modelMatrix, viewMatrix, perspectiveMatrix };
    memcpy(cubeTransformationBuffer->contents(), &transformationData, sizeof(transformationData));
    simd_float4 cubeColor = simd_make_float4(0.5, 0.9, 0.7, 1.0);
    simd_float4 lightColor = simd_make_float4(1.0, 1.0, 1.0, 1.0);
    simd_float4 lightPosition = simd_make_float4(0 - 3*cos(time/1.0), 1.2,-4 + sin(time/1.0), 1);
    simd_float4 cameraPosition = simd_make_float4(0.0f, 0.0f, 1.0f, 1.0f);

    pEnc->setFragmentBytes(&cubeColor, sizeof(cubeColor), 0);
    pEnc->setFragmentBytes(&lightColor, sizeof(lightColor), 1);
    pEnc->setFragmentBytes(&lightPosition, sizeof(lightPosition), 2);
    pEnc->setFragmentBytes(&cameraPosition, sizeof(cameraPosition), 3);
    
    pEnc->setFrontFacingWinding(MTL::WindingCounterClockwise);
    pEnc->setCullMode(MTL::CullModeBack);
    // pEnc->setTriangleFillMode(MTL::TriangleFillModeLines);
    pEnc->setRenderPipelineState(metalRenderPSO);
    pEnc->setDepthStencilState(depthStencilState);
    pEnc->setVertexBuffer(cubeVertexBuffer, 0, 0);
    pEnc->setVertexBuffer(cubeTransformationBuffer, 0, 1);
    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
    NS::UInteger vertexStart = 0;
    NS::UInteger vertexCount = 36;

    pEnc->drawPrimitives(typeTriangle, vertexStart, vertexCount);

    
    matrix_float4x4 scaleMatrix = matrix4x4_scale(0.5f, 0.5f, 0.5f);
    translationMatrix = matrix4x4_translation(lightPosition[0], lightPosition[1], lightPosition[2]);
    
    modelMatrix = simd_mul(translationMatrix, scaleMatrix);
        
    pEnc->setRenderPipelineState(metalLightSourceRenderPSO);

    transformationData = { modelMatrix, viewMatrix, perspectiveMatrix };
    memcpy(lightTransformationBuffer->contents(), &transformationData, sizeof(transformationData));
    pEnc->setVertexBuffer(lightVertexBuffer, 0, 0);
    pEnc->setVertexBuffer(lightTransformationBuffer, 0, 1);
    pEnc->setFragmentBytes(&lightColor, sizeof(lightColor), 0);
    pEnc->drawPrimitives(typeTriangle, vertexStart, vertexCount);
    pEnc->endEncoding();
    pCmd->presentDrawable( pView->currentDrawable() );
    pCmd->commit();
    pPool->release();
}

