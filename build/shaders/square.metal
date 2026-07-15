
#include <metal_stdlib>
using namespace metal;

#include <simd/simd.h>

using namespace simd;



struct NVertexData {
	float3 Position;
	float3 Normal;
    float2 TexCoords;
	float3 Tangent;
	float3 Bitangent;
};


struct N_MVP{
    matrix_float4x4 M;
    matrix_float4x4 V;
    matrix_float4x4 P;
};


struct VertexData {
    float4 position;
    float2 textureCoordinate;
};

struct MVP{
    matrix_float4x4 MVP;
};

struct Uniforms
{
    float2 time;
};

struct N_Uniforms{
     float3 lightPos;
     float3 viewPos;
     float3x3 normalMatrix;
};


struct VertexOut {
    float4 position [[position]];
    float3 FragPos ;
    float2 TexCoords;
    float3 TangentLightPos;
    float3 TangentViewPos;
    float3 TangentFragPos;
};


struct AAPLVertex {
    float2 position;
    float4 color;
    float2 textureCoordinate;
   
};


struct AAPLOut {
    float4 position [[position]];
    float4 color;
    float2 textureCoordinate;
};




vertex AAPLOut vertexRenderPass(uint vertexID [[vertex_id]],
                                constant AAPLVertex* vertexData [[buffer(0)]]) {
    AAPLOut out;
    out.position = float4(vertexData[vertexID].position, 0.0, 1.0);
    out.color = vertexData[vertexID].color;
    out.textureCoordinate = vertexData[vertexID].textureCoordinate;
    return out;
}

fragment float4 fragmentRenderPass(AAPLOut in [[stage_in]],
                                   texture2d<float> colorTex [[texture(0)]],
                                   texture2d<float> maskTex [[texture(1)]]) {

    constexpr sampler textureSampler(mag_filter::nearest, min_filter::nearest);
    float4 Original = colorTex.sample(textureSampler, in.textureCoordinate);

    float weight[5] = {0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162};

    // Scale this up to widen the glow radius. Start around 4-8 and tune visually.
    const float blurSize = 6.0;
    float2 tex_offset = blurSize * float2(1.0 / maskTex.get_width(), 1.0 / maskTex.get_height());

    // Grayscale first
    float3 centerSample = maskTex.sample(textureSampler, in.textureCoordinate).rgb;
    float centerGray = (centerSample.r + centerSample.g + centerSample.b) / 3.0;

    float result = centerGray * weight[0] * weight[0];

    // Combined horizontal + vertical taps (approximate 2D Gaussian via separable weights)
    for (int i = 1; i < 5; ++i) {
        // horizontal
        float3 hPos = maskTex.sample(textureSampler, in.textureCoordinate + float2(tex_offset.x * i, 0.0)).rgb;
        float3 hNeg = maskTex.sample(textureSampler, in.textureCoordinate - float2(tex_offset.x * i, 0.0)).rgb;
        result += ((hPos.r + hPos.g + hPos.b) / 3.0) * weight[i] * weight[0];
        result += ((hNeg.r + hNeg.g + hNeg.b) / 3.0) * weight[i] * weight[0];

        // vertical
        float3 vPos = maskTex.sample(textureSampler, in.textureCoordinate + float2(0.0, tex_offset.y * i)).rgb;
        float3 vNeg = maskTex.sample(textureSampler, in.textureCoordinate - float2(0.0, tex_offset.y * i)).rgb;
        result += ((vPos.r + vPos.g + vPos.b) / 3.0) * weight[i] * weight[0];
        result += ((vNeg.r + vNeg.g + vNeg.b) / 3.0) * weight[i] * weight[0];

        // diagonals (rough corner coverage so it doesn't look cross-shaped)
        float3 dPos = maskTex.sample(textureSampler, in.textureCoordinate + float2(tex_offset.x * i, tex_offset.y * i)).rgb;
        float3 dNeg = maskTex.sample(textureSampler, in.textureCoordinate - float2(tex_offset.x * i, tex_offset.y * i)).rgb;
        result += ((dPos.r + dPos.g + dPos.b) / 3.0) * weight[i] * weight[i];
        result += ((dNeg.r + dNeg.g + dNeg.b) / 3.0) * weight[i] * weight[i];
    }

    float3 bloom = float3(result, result, result);
    float3 hdrColor = Original.rgb + bloom;
    float3 mapped = float3(1.0f) - exp(-hdrColor * 17.5f);

    return float4(mapped, 1.0f);
}

vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                              constant NVertexData* vertexData [[buffer(0)]],
                              constant N_Uniforms& uniforms [[buffer(1)]],
                              constant N_MVP & mvp [[buffer(2)]] ) {
        VertexOut vs_out;
        // out.position = mvp.MVP * vertexData[vertexID].position;
        // out.textureCoordinate = vertexData[vertexID].textureCoordinate;

        vs_out.FragPos = float3(mvp.M * float4(vertexData[vertexID].Position, 1.0));   
        vs_out.TexCoords = vertexData[vertexID].TexCoords;
        float3x3 normalMatrix = uniforms.normalMatrix;
        float3 T = normalize(normalMatrix * vertexData[vertexID].Tangent);
        float3 N = normalize(normalMatrix * vertexData[vertexID].Normal);
        T = normalize(T - dot(T, N) * N);
        float3 B = cross(N, T);
        float3x3 TBN = transpose(float3x3(T, B, N));    
        vs_out.TangentLightPos = TBN * uniforms.lightPos;
        vs_out.TangentViewPos  = TBN * uniforms.viewPos;
        vs_out.TangentFragPos  = TBN * vs_out.FragPos;
        vs_out.position = mvp.P * mvp.V * mvp.M * float4(vertexData[vertexID].Position, 1.0);


        return vs_out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> DiffuseMap [[texture(0)]],
                               texture2d<float> NormalMap [[texture(1)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    

    float3 normal = NormalMap.sample(textureSampler, in.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    // normal = (normal * 2.0 - 1.0);
    // normal.xy *= 1000;   
    // normal = normalize(normal);

    float3 color = DiffuseMap.sample(textureSampler, in.TexCoords).rgb;
    float3 ambient = 0.1 * color;
    float3 lightDir = normalize(in.TangentLightPos - in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    float3 diffuse = diff * color;
    float3 viewDir = normalize(in.TangentViewPos - in.TangentFragPos);
    // float3 reflectDir = reflect(-lightDir, normal);
    float3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    float3 specular = float3(0.2) * spec;

    // return float4(color, 1.0);
     

    return float4(ambient + diffuse + specular, 1.0);
}


