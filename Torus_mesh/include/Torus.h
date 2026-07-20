#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>




#include <vector>
#include <cmath>
#include <numbers>




class Torus {
public:

     Torus() = default;
    ~Torus() = default;

     
void generateTorus(float majorRadius, float minorRadius, int majorSegments, int minorSegments,
                   std::vector<glm::vec3>& outVertices,
                   std::vector<glm::vec2>& outUVs,
                   std::vector<unsigned int>& outIndices) {

    const float PI = std::numbers::pi_v<float>;

    for (int i = 0; i <= majorSegments; ++i) {
        float theta = i * (2.0f * PI / majorSegments); 
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        for (int j = 0; j <= minorSegments; ++j) {
            float phi = j * (2.0f * PI / minorSegments); 
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);

            
           
            float x = (majorRadius + minorRadius * cosPhi) * cosTheta;
            float y = (majorRadius + minorRadius * cosPhi) * sinTheta;
            float z = minorRadius * sinPhi;
            
            outVertices.push_back({x, y, z});

            
            glm::vec2 uv;
            uv[0] = (float)i / majorSegments;
            uv[1] = (float)j / minorSegments;
            outUVs.push_back(uv);
        }
    }

    
    for (int i = 0; i < majorSegments; ++i) {
        for (int j = 0; j < minorSegments; ++j) {
            unsigned int current = i * (minorSegments + 1) + j;
            unsigned int next = (i + 1) * (minorSegments + 1) + j;

            
            outIndices.push_back(current);
            outIndices.push_back(next);
            outIndices.push_back(current + 1);

            
            outIndices.push_back(current + 1);
            outIndices.push_back(next);
            outIndices.push_back(next + 1);
        }
    }
}
    


};