#include "Renderer.hpp"

void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();

    // ─── Compute Pass ─────────────────────────────────────────────────────────
    MTL::ComputePassDescriptor* CCPD = MTL::ComputePassDescriptor::computePassDescriptor();
    MTL::ComputeCommandEncoder* CCE = pCmd->computeCommandEncoder(CCPD);

    CCE->setComputePipelineState(_pPSO);
    CCE->setBuffer(__MBufferA,      0, 0);
    CCE->setBuffer(__MBufferB,      0, 1);
    CCE->setBuffer(__MBufferResult, 0, 2);

    MTL::Size gridSize       = MTL::Size(arraylenght, 1, 1);
    NS::UInteger tgSize      = _pPSO->maxTotalThreadsPerThreadgroup();
    if (tgSize > arraylenght) tgSize = arraylenght;
    MTL::Size threadGroupSize = MTL::Size((uint32_t)tgSize, 1, 1);

    CCE->dispatchThreads(gridSize, threadGroupSize);
    CCE->endEncoding();

    // ─── Render Pass ──────────────────────────────────────────────────────────
    MTL::RenderPassDescriptor* RPD = pView->currentRenderPassDescriptor();
    if (RPD)
    {
        // Clear to a dark charcoal colour
        RPD->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(0.1, 0.1, 0.1, 1.0));
        RPD->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        RPD->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

        MTL::RenderCommandEncoder* RCE = pCmd->renderCommandEncoder(RPD);

        RCE->setRenderPipelineState(_pRPSO);

        // Draw a fullscreen triangle (3 verts, no vertex buffer needed)
        RCE->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));

        RCE->endEncoding();

        // Present the drawable
        pCmd->presentDrawable(pView->currentDrawable());
    }

    pCmd->commit();
    pCmd->waitUntilCompleted();


    verifyResult();

    pPool->release();
}

Renderer::Renderer(MTL::Device* pDevice) : _pDevice(pDevice->retain())
{
    _pCommandQueue = _pDevice->newCommandQueue();
    assert(_pCommandQueue && "Failed to create command queue");

    __MBufferA      = _pDevice->newBuffer(bufferSize, MTL::ResourceStorageModeShared);
    __MBufferB      = _pDevice->newBuffer(bufferSize, MTL::ResourceStorageModeShared);
    __MBufferResult = _pDevice->newBuffer(bufferSize, MTL::ResourceStorageModeShared);

    assert(__MBufferA      && "Failed to allocate MBufferA");
    assert(__MBufferB      && "Failed to allocate MBufferB");
    assert(__MBufferResult && "Failed to allocate MBufferResult");

    generateRandomFloatData(__MBufferA);
    generateRandomFloatData(__MBufferB);

    makePipeline();
    assert(_pPSO  && "Failed to create compute pipeline state");
    assert(_pRPSO && "Failed to create render pipeline state");
}

Renderer::~Renderer()
{
    __MBufferA->release();
    __MBufferB->release();
    __MBufferResult->release();
    _pPSO->release();
    _pRPSO->release();
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::makePipeline()
{
    using NS::StringEncoding::UTF8StringEncoding;

    // ─── Compute Shader ───────────────────────────────────────────────────────
    const char* computeSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        kernel void add_arrays(device const float* inA,
                               device const float* inB,
                               device float*       result,
                               uint index [[thread_position_in_grid]])
        {
            result[index] = inA[index] + inB[index];
        }
    )";

    // ─── Vertex + Fragment Shaders ────────────────────────────────────────────
    const char* renderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct VertexOut {
            float4 position [[position]];
            float4 color;
        };

        // Fullscreen triangle — no vertex buffer needed
        vertex VertexOut vert_main(uint vid [[vertex_id]])
        {
            // Covers the entire clip space with a single triangle
            float2 positions[3] = {
                float2(-1.0, -1.0),
                float2( 3.0, -1.0),
                float2(-1.0,  3.0)
            };

            VertexOut out;
            out.position = float4(positions[vid], 0.0, 1.0);
            out.color    = float4(0.2, 0.4, 0.8, 1.0); // solid blue
            return out;
        }

        fragment float4 frag_main(VertexOut in [[stage_in]])
        {
            return in.color;
        }
    )";

    NS::Error* pError = nullptr;

    // Compile compute library
    MTL::Library* pComputeLib = _pDevice->newLibrary(
        NS::String::string(computeSrc, UTF8StringEncoding), nullptr, &pError);
    if (!pComputeLib) {
        __builtin_printf("Compute library error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* computeFn = pComputeLib->newFunction(
        NS::String::string("add_arrays", UTF8StringEncoding));
    assert(computeFn && "Failed to find kernel add_arrays");

    _pPSO = _pDevice->newComputePipelineState(computeFn, &pError);
    computeFn->release();
    pComputeLib->release();
    if (!_pPSO) {
        __builtin_printf("Compute PSO error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }

    // Compile render library
    MTL::Library* pRenderLib = _pDevice->newLibrary(
        NS::String::string(renderSrc, UTF8StringEncoding), nullptr, &pError);
    if (!pRenderLib) {
        __builtin_printf("Render library error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* vertFn = pRenderLib->newFunction(
        NS::String::string("vert_main", UTF8StringEncoding));
    MTL::Function* fragFn = pRenderLib->newFunction(
        NS::String::string("frag_main", UTF8StringEncoding));
    assert(vertFn && "Failed to find vert_main");
    assert(fragFn && "Failed to find frag_main");

    MTL::RenderPipelineDescriptor* pRPD = MTL::RenderPipelineDescriptor::alloc()->init();
    pRPD->setVertexFunction(vertFn);
    pRPD->setFragmentFunction(fragFn);
    pRPD->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    _pRPSO = _pDevice->newRenderPipelineState(pRPD, &pError);

    vertFn->release();
    fragFn->release();
    pRPD->release();
    pRenderLib->release();

    if (!_pRPSO) {
        __builtin_printf("Render PSO error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }
}

void Renderer::generateRandomFloatData(MTL::Buffer* buffer)
{
    float* array = (float*)buffer->contents();
    for (size_t i = 0; i < arraylenght; ++i)
        array[i] = (float)rand() / (float)(RAND_MAX);
}

void Renderer::verifyResult()
{
    float* a = (float*)__MBufferA->contents();
    float* b = (float*)__MBufferB->contents();
    float* r = (float*)__MBufferResult->contents();

    for (size_t i = 0; i < arraylenght; ++i)
    {
        if (r[i] != a[i] + b[i])
        {
            __builtin_printf("FAILED at index %zu: %.6f + %.6f = %.6f, got %.6f\n",
                             i, a[i], b[i], a[i] + b[i], r[i]);
            return;
        }
    }
    __builtin_printf("Success\n");
}