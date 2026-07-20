
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>

#include <OBJ_Loader.h>



// Only works for 1 mesh 
class Mesh
{

public:
        uint32_t indexCount = 0;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        Mesh(const std::string &filepath)
        {

                objl::Loader Loader;
                bool loadout = Loader.LoadFile(filepath);

                if (loadout)
                {
                        //for (int i = 0; i < Loader.LoadedMeshes.size(); i++) 

                        objl::Mesh curMesh = Loader.LoadedMeshes[0]; // replace with i 
                        for (int j = 0; j < curMesh.Vertices.size(); j++){
                                positions.emplace_back(glm::vec3(curMesh.Vertices[j].Position.X, curMesh.Vertices[j].Position.Y, curMesh.Vertices[j].Position.Z));
                                uv.emplace_back(glm::vec2(curMesh.Vertices[j].TextureCoordinate.X, curMesh.Vertices[j].TextureCoordinate.Y));
                                normals.emplace_back(glm::vec3(curMesh.Vertices[j].Normal.X, curMesh.Vertices[j].Normal.Y, curMesh.Vertices[j].Normal.Z));
                        }

                        this->indexCount = curMesh.Indices.size();
                        for (int j = 0; j < indexCount; j += 3){
				indices.emplace_back(curMesh.Indices[j]);
                                indices.emplace_back(curMesh.Indices[j+1]);
                                indices.emplace_back(curMesh.Indices[j+2]) ;
			}

                        
                }
        }
};