
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

float2 ParallaxMapping(float2 texCoords, float3 viewDir,
                       texture2d<float> depthMap, float heightScale)
{ 

    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);


    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 64;
    float numLayers = mix(maxLayers, minLayers, abs(dot(float3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    float2 P = viewDir.xy / viewDir.z * heightScale; 
    float2 deltaTexCoords = P / numLayers;
  
    
    float2  currentTexCoords     = texCoords;
    float currentDepthMapValue = depthMap.sample(textureSampler, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        
        currentTexCoords -= deltaTexCoords;
        
        currentDepthMapValue = depthMap.sample(textureSampler, currentTexCoords).r;  
        
        currentLayerDepth += layerDepth;  
    }
    
   
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;

    
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = depthMap.sample(textureSampler, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    
    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]],
                               texture2d<float> DiffuseMap [[texture(0)]],
                               texture2d<float> NormalMap [[texture(1)]],
                               texture2d<float> DepthMap [[texture(2)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    

    float3 normal;

    float3 viewDir = normalize(in.TangentViewPos - in.TangentFragPos);
    float2 texCoords = in.TexCoords;
    float heightScale = 0.5f; // Adjust this value to control the depth effect
    
    texCoords = ParallaxMapping(in.TexCoords,  viewDir, DepthMap, heightScale);  

    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard_fragment();

    // obtain normal from normal map
    normal = NormalMap.sample(textureSampler, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);   
   
    // get diffuse color
    float3 color = DiffuseMap.sample(textureSampler, texCoords).rgb;
    // ambient
    float3 ambient = 0.1 * color;
    // diffuse
    float3 lightDir = normalize(in.TangentLightPos - in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    float3 diffuse = diff * color;
    // specular    
    float3 reflectDir = reflect(-lightDir, normal);
    float3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    float3 specular = float3(0.2) * spec;
    return float4(ambient + diffuse + specular, 1.0);

   

}


