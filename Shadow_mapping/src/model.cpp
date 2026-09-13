




#include "model.hpp"

#include <iostream>

Model::Model(std::string filePath, MTL::Device* metalDevice) {
    device = metalDevice;
    baseDirectory = filePath.substr(0, filePath.find_last_of("/\\") + 1); 
    loadModel(filePath);
}

Model::~Model() {
    std::cout << "Model->release()" << std::endl;
    delete textures;
    for (auto mesh : meshes)
        delete mesh;
}

void Model::loadModel(std::string& filePath) {
    Assimp::Importer assimpImporter;
    const aiScene* scene = assimpImporter.ReadFile(filePath.c_str(),
                                                   aiProcess_Triangulate      |
                                                   aiProcess_CalcTangentSpace
                                                   );
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error: " << assimpImporter.GetErrorString() << std::endl;
        
    }
    
    loadTextures(scene);
    processNode(scene->mRootNode, scene);
}

void Model::loadTextures(const aiScene* scene) {
    std::cout << "Loading Textures..." << std::endl;
    int textureIndex = 0;
    for (int i = 0;  i < scene->mNumMaterials; i++) {
        aiMaterial* material = scene->mMaterials[i];
        mapTextureIndices(aiTextureType_DIFFUSE, material, textureIndex);
        mapTextureIndices(aiTextureType_SPECULAR, material, textureIndex);
        mapTextureIndices(aiTextureType_HEIGHT, material, textureIndex);
        mapTextureIndices(aiTextureType_EMISSIVE, material, textureIndex);
    }
    if (textureFilePaths.size() == 0) {
        std::cerr << "Texture Files not found..." << std::endl;
        exit(1);
    }
    textures = new TextureArray(textureFilePaths,
                                device);
}

void Model::mapTextureIndices(aiTextureType textureType, aiMaterial* material, int& textureIndex) {
    for (int j = 0; j < material->GetTextureCount(textureType); j++) {
        aiString textureFileName;
        if (material->GetTexture(textureType, j, &textureFileName) == AI_SUCCESS) {
            std::string key = textureFileName.C_Str();
            
            if (textureIndexMap.count(key) == 0) {
                std::string textureFilePath = baseDirectory + key;
                std::cout << textureIndex + 1 << ".) " << textureFilePath << std::endl;
                textureIndexMap[key] = textureIndex++;
                textureFilePaths.push_back(textureFilePath);
            }
        }
    }
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* aiMesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(aiMesh, scene));
    }
    
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}
Mesh* Model::processMesh(aiMesh* aiMesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    aiString diffuseName, specularName, normalName, emissiveName;
    aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];

    
    if (material->GetTextureCount(aiTextureType_DIFFUSE)  > 0)
        material->GetTexture(aiTextureType_DIFFUSE,  0, &diffuseName);
    if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
        material->GetTexture(aiTextureType_SPECULAR, 0, &specularName);
    if (material->GetTextureCount(aiTextureType_HEIGHT)   > 0)
        material->GetTexture(aiTextureType_HEIGHT,   0, &normalName);
    if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
        material->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveName);

    
    uint32_t diffuseIdx  = resolveTextureIndex(diffuseName);
    uint32_t specularIdx = resolveTextureIndex(specularName);
    uint32_t normalIdx   = resolveTextureIndex(normalName);
    uint32_t emissiveIdx = resolveTextureIndex(emissiveName);

    for (unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
        aiFace face = aiMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            unsigned int vi = face.mIndices[j];

            Vertex vertex;
            vertex.position          = { aiMesh->mVertices[vi].x,  aiMesh->mVertices[vi].y,  aiMesh->mVertices[vi].z  };
            vertex.normal            = { aiMesh->mNormals[vi].x,   aiMesh->mNormals[vi].y,   aiMesh->mNormals[vi].z   };
            vertex.textureCoordinate = { aiMesh->mTextureCoords[0][vi].x, aiMesh->mTextureCoords[0][vi].y };
            vertex.tangent           = { aiMesh->mTangents[vi].x,  aiMesh->mTangents[vi].y,  aiMesh->mTangents[vi].z  };
            vertex.bitangent         = { aiMesh->mBitangents[vi].x, aiMesh->mBitangents[vi].y, aiMesh->mBitangents[vi].z }; 

            vertex.diffuseTextureIndex  = diffuseIdx;
            vertex.specularTextureIndex = specularIdx;
            vertex.normalMapIndex       = normalIdx;
            vertex.emissiveMapIndex     = emissiveIdx;

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }
    return new Mesh(vertices, indices, device);
}



static constexpr uint32_t NO_TEXTURE = UINT32_MAX;

uint32_t Model::resolveTextureIndex(const aiString& name) {
    if (name.length == 0) return NO_TEXTURE;
    auto it = textureIndexMap.find(name.C_Str());
    if (it == textureIndexMap.end()) return NO_TEXTURE;
    return it->second;
}