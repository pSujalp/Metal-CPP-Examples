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

    __builtin_printf("Step 4: CreateCube\n");
    CreateCube();

    __builtin_printf("Step 5: constructor done\n");
}

Renderer::~Renderer()
{
    cubeVertexBuffer->release();   
    delete D_Texture;
    _pPSO->release();
    depthStencilState->release();
    UniformBuffer->release();
    transformationBuffer->release();
    _pCommandQueue->release();
    _pDevice->release();
}

void Renderer::createDefaultLibrary(MTL::Device* pDevice) {
    using NS::StringEncoding::UTF8StringEncoding;

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    __builtin_printf("CWD: %s\n", cwd);

    Shader sh;
    const char* shadersrc = sh.GetShader("shaders/square.metal");
    if (!shadersrc) {
        __builtin_printf("ERROR: GetShader returned null — file not found\n");
        assert(false);
    }
    __builtin_printf("Shader source loaded, first 100 chars:\n%.100s\n", shadersrc);

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

    NS::Array* fnames = metallibrary->functionNames();
    __builtin_printf("Function count: %lu\n", fnames->count());
    for (uint32_t i = 0; i < fnames->count(); ++i) {
        __builtin_printf("  fn: %s\n", ((NS::String*)fnames->object(i))->utf8String());
    }
}

void Renderer::CreateCube() {


    glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
	glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
	glm::vec3 pos3(1.0f, -1.0f, 0.0f);
	glm::vec3 pos4(1.0f, 1.0f, 0.0f);
	// texture coordinates
	glm::vec2 uv1(0.0f, 1.0f);
	glm::vec2 uv2(0.0f, 0.0f);
	glm::vec2 uv3(1.0f, 0.0f);
	glm::vec2 uv4(1.0f, 1.0f);
	// normal vector
	glm::vec3 nm(0.0f, 0.0f, 1.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent1, bitangent1;
	glm::vec3 tangent2, bitangent2;
	// triangle 1
	// ----------
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

	// triangle 2
	// ----------
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

	// NOTE: previously these used parentheses, e.g. (pos1.x, pos1.y, pos1.z),
	// which is a comma-expression that evaluates to just pos1.z broadcast
	// into the vector (all positions collapsed to (0,0,0) since z==0 for
	// every corner). Using brace-init-lists fixes this.
	vertexData.emplace_back(NVertexData{ {pos1.x, pos1.y, pos1.z}, {nm.x, nm.y, nm.z}, {uv1.x, uv1.y}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z} });
	vertexData.emplace_back(NVertexData{ {pos2.x, pos2.y, pos2.z}, {nm.x, nm.y, nm.z}, {uv2.x, uv2.y}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z} });
	vertexData.emplace_back(NVertexData{ {pos3.x, pos3.y, pos3.z}, {nm.x, nm.y, nm.z}, {uv3.x, uv3.y}, {tangent1.x, tangent1.y, tangent1.z}, {bitangent1.x, bitangent1.y, bitangent1.z} });
	vertexData.emplace_back(NVertexData{ {pos1.x, pos1.y, pos1.z}, {nm.x, nm.y, nm.z}, {uv1.x, uv1.y}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z} });
	vertexData.emplace_back(NVertexData{ {pos3.x, pos3.y, pos3.z}, {nm.x, nm.y, nm.z}, {uv3.x, uv3.y}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z} });
	vertexData.emplace_back(NVertexData{ {pos4.x, pos4.y, pos4.z}, {nm.x, nm.y, nm.z}, {uv4.x, uv4.y}, {tangent2.x, tangent2.y, tangent2.z}, {bitangent2.x, bitangent2.y, bitangent2.z} });


    planeVertexBuffer = _pDevice->newBuffer(
        vertexData.data(),
        sizeof(NVertexData) * vertexData.size(),
        MTL::ResourceStorageModeShared
    );
    D_Texture = new Texture("assets/bricks2.jpg", _pDevice);
    N_Texture = new Texture("assets/bricks2_normal.jpg", _pDevice);
    Disp_Texture = new Texture("assets/bricks2_disp.jpg", _pDevice);
}

void Renderer::buildShaders()
{
    NS::Error* pError = nullptr;

    MTL::Function* vertexShader = metallibrary->newFunction(
        NS::String::string("vertexShader", NS::ASCIIStringEncoding)
    );
    if (!vertexShader) {
        __builtin_printf("ERROR: vertex function 'vertexShader' not found in library.\n");
        assert(false);
    }
    MTL::Function* fragmentShader = metallibrary->newFunction(
        NS::String::string("fragmentShader", NS::ASCIIStringEncoding)
    );
    if (!fragmentShader) {
        __builtin_printf("ERROR: fragment function 'fragmentShader' not found in library.\n");
        assert(false);
    }
    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(vertexShader);
    pDesc->setFragmentFunction(fragmentShader);
    pDesc->colorAttachments()->object(0)->setPixelFormat(
        MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB
    );
    
    pDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    MTL::DepthStencilDescriptor* depthStencilDescriptor =
        MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = _pDevice->newDepthStencilState(depthStencilDescriptor);
    depthStencilDescriptor->release();

    _pPSO = _pDevice->newRenderPipelineState(pDesc, &pError);
    if (!_pPSO) {
        __builtin_printf("%s", pError->localizedDescription()->utf8String());
        assert(false);
    }

    UniformBuffer       = _pDevice->newBuffer(sizeof(N_Uniforms), MTL::ResourceStorageModeShared);
    transformationBuffer = _pDevice->newBuffer(sizeof(N_MVP),     MTL::ResourceStorageModeShared);

    fragmentShader->release();
    vertexShader->release();
    pDesc->release();
}

void Renderer::draw(MTK::View* pView)
{
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

    
    

    
    
    glm::mat4 model = glm::mat4(1.0f);
    // NOTE: was translated to (0, -9, 0), which put the quad far below the
    // camera's frustum (camera sits at (0,0,5) looking at the origin).
    // Keeping it at the origin so it's actually in view.
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

    
    static float accumulatedDegrees = 0.0f;
    const float rotationSpeedDegreesPerSecond = 45.0f;
    accumulatedDegrees += rotationSpeedDegreesPerSecond * Time::DeltaTime;
    if (accumulatedDegrees >= 360.0f)
        accumulatedDegrees -= 360.0f;

    
    float angleInRadians = accumulatedDegrees * (M_PI / 180.0f);
    // model = glm::rotate(model, angleInRadians, glm::vec3(0.0f, 1.0f, 0.0f));
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

    N_MVP mvp1;
    mvp1.M = matrix_float4x4({
        simd::float4{ model[0][0], model[0][1], model[0][2], model[0][3] },
        simd::float4{ model[1][0], model[1][1], model[1][2], model[1][3] },
        simd::float4{ model[2][0], model[2][1], model[2][2], model[2][3] },
        simd::float4{ model[3][0], model[3][1], model[3][2], model[3][3] },
    });
    mvp1.V = matrix_float4x4({
        simd::float4{ viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2], viewMatrix[0][3] },
        simd::float4{ viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2], viewMatrix[1][3] },
        simd::float4{ viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2], viewMatrix[2][3] },
        simd::float4{ viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2], viewMatrix[3][3] },
    });
    mvp1.P = matrix_float4x4({
        simd::float4{ perspectiveMatrix[0][0], perspectiveMatrix[0][1], perspectiveMatrix[0][2], perspectiveMatrix[0][3] },
        simd::float4{ perspectiveMatrix[1][0], perspectiveMatrix[1][1], perspectiveMatrix[1][2], perspectiveMatrix[1][3] },
        simd::float4{ perspectiveMatrix[2][0], perspectiveMatrix[2][1], perspectiveMatrix[2][2], perspectiveMatrix[2][3] },
        simd::float4{ perspectiveMatrix[3][0], perspectiveMatrix[3][1], perspectiveMatrix[3][2], perspectiveMatrix[3][3] },
    });

    memcpy(transformationBuffer->contents(), &mvp1, sizeof(N_MVP));


    glm::mat4 cameraToWorld = glm::inverse(viewMatrix);
    glm::vec3 cameraPos = glm::vec3(cameraToWorld[3]); 
    float3 mslVec1 = *reinterpret_cast<float3*>(&cameraPos);

    glm::mat3 normalMatrix = glm::transpose(inverse(glm::mat3(model)));
    N_Uniforms uniforms;

    // NOTE: was float3{(0.0f, 1.5f, -1.0f)} — same comma-expression bug,
    // which collapsed this to (-1,-1,-1). Fixed with brace-init.
    //
    // Also moved from z=-1 to z=+2: the quad's normal is (0,0,1), so a
    // light behind the surface (negative z) gives dot(lightDir,normal) < 0
    // everywhere, which clamps to 0 in max(dot(...), 0.0) — diffuse and
    // specular both vanish and you only ever see the flat 10% ambient
    // term (no visible bumps). Placing the light in front, and offset in
    // x/y for a grazing angle, is what actually reveals normal-map detail
    // — a light straight down the normal lights every bump almost equally.
    uniforms.lightPos = float3{1.5f, 1.5f, 2.0f};
    uniforms.viewPos  = mslVec1;
    uniforms.normalMatrix = float3x3{
        simd::float3{ normalMatrix[0][0], normalMatrix[0][1], normalMatrix[0][2] },
        simd::float3{ normalMatrix[1][0], normalMatrix[1][1], normalMatrix[1][2] },
        simd::float3{ normalMatrix[2][0], normalMatrix[2][1], normalMatrix[2][2] }
    };
    memcpy(UniformBuffer->contents(), &uniforms, sizeof(N_Uniforms));


    


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

    pEnc->setRenderPipelineState(_pPSO);
    pEnc->setDepthStencilState(depthStencilState);

    pEnc->setVertexBuffer(planeVertexBuffer,      0, 0);  
    pEnc->setVertexBuffer(UniformBuffer,         0, 1);  
    pEnc->setVertexBuffer(transformationBuffer,  0, 2);  

    pEnc->setFragmentTexture(D_Texture->texture, 0);
    pEnc->setFragmentTexture(N_Texture->texture, 1);
    pEnc->setFragmentTexture(Disp_Texture->texture, 2);

    pEnc->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(6));

    pEnc->endEncoding();
    pCmd->presentDrawable(pView->currentDrawable());
    pCmd->commit();

    pPool->release();
}