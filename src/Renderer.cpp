#include "Renderer.hpp"

void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();

    // Create compute pass descriptor and encoder
    MTL::ComputePassDescriptor* CCPD = MTL::ComputePassDescriptor::computePassDescriptor();
    MTL::ComputeCommandEncoder* CCE = pCmd->computeCommandEncoder(CCPD);
    CCPD->release();

    // Bind pipeline state
    CCE->setComputePipelineState(_pPSO);

    // Bind input and output buffers to argument table indices 0, 1, 2
    CCE->setBuffer(__MBufferA, 0, 0);
    CCE->setBuffer(__MBufferB, 0, 1);
    CCE->setBuffer(__MBufferResult, 0, 2);

    // Total number of threads to dispatch (one thread per element)
    MTL::Size gridSize = MTL::Size(arraylenght, 1, 1);

    // Clamp thread group size to array length if needed
    NS::UInteger threadGroupSize = _pPSO->maxTotalThreadsPerThreadgroup();
    if (threadGroupSize > arraylenght) threadGroupSize = arraylenght;
    MTL::Size threadGroupsize = MTL::Size((uint32_t)threadGroupSize, 1, 1);

    // Dispatch threads
    CCE->dispatchThreads(gridSize, threadGroupsize);

    // Finish encoding — do NOT release CCE, command buffer owns it
    CCE->endEncoding();

    // Commit and wait
    pCmd->commit();
    pCmd->waitUntilCompleted();
    pCmd->release();

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
    assert(_pPSO && "Failed to create pipeline state");
}

Renderer::~Renderer()
{
    __MBufferA->release();
    __MBufferB->release();
    __MBufferResult->release();
    _pPSO->release();
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::makePipeline()
{
    using NS::StringEncoding::UTF8StringEncoding;

    const char* shaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        kernel void add_arrays(device const float* inA,
                               device const float* inB,
                               device float* result,
                               uint index [[thread_position_in_grid]])
        {
            result[index] = inA[index] + inB[index];
        }
    )";

    NS::Error* pError = nullptr;

    MTL::Library* pLibrary = _pDevice->newLibrary(
        NS::String::string(shaderSrc, UTF8StringEncoding),
        nullptr,
        &pError
    );
    if (!pLibrary)
    {
        __builtin_printf("Library error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* kernel_add_array = pLibrary->newFunction(
        NS::String::string("add_arrays", UTF8StringEncoding)
    );
    assert(kernel_add_array && "Failed to find kernel function add_arrays");

    _pPSO = _pDevice->newComputePipelineState(kernel_add_array, &pError);
    kernel_add_array->release();

    if (!_pPSO)
    {
        __builtin_printf("PSO error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }

    pLibrary->release();
}

void Renderer::generateRandomFloatData(MTL::Buffer* buffer)
{
    // Write directly into shared buffer — no heap allocation needed
    float* array = (float*)buffer->contents();
    for (size_t i = 0; i < arraylenght; ++i)
    {
        array[i] = (float)rand() / (float)(RAND_MAX);
    }
}

void Renderer::verifyResult()
{
    // Read directly from shared buffers
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