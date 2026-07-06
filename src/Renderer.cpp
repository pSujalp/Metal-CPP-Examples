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

    MatMulParams *params = (MatMulParams *)m_device_buffer_params_ptr->contents();
    params->row_dim_x = m_rows_X;
    params->col_dim_x = m_cols_X;
    params->inner_dim = m_cols_A;

    CCE->setComputePipelineState(m_MatMultiplyFunctionPSO);
    CCE->setBuffer(m_device_buffer_A_ptr, 0, 0);
    CCE->setBuffer(m_device_buffer_B_ptr, 0, 1);
    CCE->setBuffer(m_device_buffer_X_ptr, 0, 2);
    CCE->setBuffer(m_device_buffer_params_ptr, 0, 3);

    const int x_threads_per_group = 8;
    const int y_threads_per_group = 8;
    assert(x_threads_per_group == y_threads_per_group);
const int x_group_count = (m_cols_X + x_threads_per_group - 1) / x_threads_per_group; // (4+7)/8 = 1
const int y_group_count = (m_rows_X + y_threads_per_group - 1) / y_threads_per_group; // (4+7)/8 = 1
    MTL::Size thread_group_count = MTL::Size::Make(x_group_count, y_group_count, 1);          // should be the size of the grid = (x_threads, y_threads)
    MTL::Size threadgroupSize = MTL::Size::Make(x_threads_per_group, y_threads_per_group, 1); //

    CCE->dispatchThreadgroups(thread_group_count, threadgroupSize);

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

    m_device_buffer_A_ptr = pDevice->newBuffer(m_rows_X * m_cols_A * sizeof(float), MTL::ResourceStorageModeShared);
    m_device_buffer_B_ptr = pDevice->newBuffer(m_cols_A * m_cols_X * sizeof(float), MTL::ResourceStorageModeShared);
    m_device_buffer_X_ptr = pDevice->newBuffer(m_rows_X * m_cols_X * sizeof(float), MTL::ResourceStorageModeShared);
    m_device_buffer_params_ptr = pDevice->newBuffer(sizeof(MatMulParams), MTL::ResourceStorageModeShared);


    Matrix<float> A(static_cast<float*>(m_device_buffer_A_ptr->contents()), {m_rows_X, m_cols_A});
    Matrix<float> B(static_cast<float*>(m_device_buffer_B_ptr->contents()), {m_cols_A, m_cols_X});
    
    // Let's randomize the two input matricies.
    // This runs on the CPU (refer to the implementation in Utilities.cpp)
    randomize_uniform(A, -1.0f, 1.0f);
    randomize_uniform(B, -1.0f, 1.0f);
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
    if (!_pPSO)
    {
        __builtin_printf("Compute PSO error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }

    Shader sh;

    MTL::Library *pComputeLib1 = _pDevice->newLibrary(
        NS::String::string(sh.GetShader("shaders/Mutmul.metal"), UTF8StringEncoding), nullptr, &pError);
    if (!pComputeLib1)
    {
        __builtin_printf("Compute library error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function *computeFn1 = pComputeLib1->newFunction(
        NS::String::string("mat_mul_opt1", UTF8StringEncoding));
    assert(computeFn1 && "Failed to find kernel mat_mul_opt1");

    m_MatMultiplyFunctionPSO = _pDevice->newComputePipelineState(computeFn1, &pError);
    computeFn1->release();
    pComputeLib1->release();

    if (!m_MatMultiplyFunctionPSO)
    {
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
    std::cout << "Verifying result..." << std::endl;
    Matrix<float> A(static_cast<float *>(m_device_buffer_A_ptr->contents()), {m_rows_X, m_cols_A});
    Matrix<float> B(static_cast<float *>(m_device_buffer_B_ptr->contents()), {m_cols_A, m_cols_X});
    Matrix<float> X(static_cast<float *>(m_device_buffer_X_ptr->contents()), {m_rows_X, m_cols_X});
    // Show the contents if small.
    if (X.size() < 1000)
    {
        std::cout << "A:\n"
             << A << std::endl;
        std::cout << "B:\n"
             << B << std::endl;
        std::cout << "X:\n"
             << X << std::endl;
    }

    const float max_allowable_error = 1e-3;

    // Create empty matrix.
    Matrix<float> X_true;

    // Compute the true matrix product using BLAS sgemm.
    // X_true <- A x B
    mat_multiply_blas(X_true, A, B);

    const float max_error = assert_almost_equal_max_error(X, X_true, max_allowable_error);

    const float max_result_val = max_value(X_true);
    if (max_result_val == 0)
    {
        std::cout << "Max result magnitude was: " << max_result_val << std::endl;
        std::cout << "It is meaningless to verify unless some values are non-zero!" << std::endl;
        error_exit("exiting");
    }
    std::cout << "Passed! Max error was: " << max_error << std::endl;
}