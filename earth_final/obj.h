#ifndef OBJ_H
#define OBJ_H

#include <GL/glew.h> 
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader_m.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>


using namespace std;
class OBJ{
public:
    std::vector< glm::vec3 > vertices;
    std::vector< glm::vec2 > uvs;
    std::vector< glm::vec3 > normals;
    std::vector< glm::vec3> tangents;
	std::vector< glm::vec3 > bitangents;
    std::vector<int> indices;
    unsigned int VAO,VBO,EBO,uvbuffer, normalbuffer, tangentbuffer, bitangentbuffer;

    OBJ(
        const char * path
    ){

        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(path, 0/*aiProcess_JoinIdenticalVertices | aiProcess_SortByPType*/);
        if( !scene) {
            fprintf( stderr, importer.GetErrorString());
            getchar();
            return;
        }
        const aiMesh* mesh = scene->mMeshes[0];

        vertices.reserve(mesh->mNumVertices);
        for(unsigned int i=0; i<mesh->mNumVertices; i++){
            aiVector3D pos = mesh->mVertices[i];
            vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
        }

        uvs.reserve(mesh->mNumVertices);
        for(unsigned int i=0; i<mesh->mNumVertices; i++){
            aiVector3D UVW = mesh->mTextureCoords[0][i];
            uvs.push_back(glm::vec2(UVW.x, UVW.y));
        }

        normals.reserve(mesh->mNumVertices);
        for(unsigned int i=0; i<mesh->mNumVertices; i++){
            aiVector3D n = mesh->mNormals[i];
            normals.push_back(glm::vec3(n.x, n.y, n.z));
        }


        indices.reserve(3*mesh->mNumFaces);
        for (unsigned int i=0; i<mesh->mNumFaces; i++){
            indices.push_back(mesh->mFaces[i].mIndices[0]);
            indices.push_back(mesh->mFaces[i].mIndices[1]);
            indices.push_back(mesh->mFaces[i].mIndices[2]);
        }
        computeTangentBasis(
		vertices, uvs, normals, 
		tangents, bitangents);
        setup();
    }

    void flush(){
        glDeleteVertexArrays(1, &VAO);
        glDeleteVertexArrays(1, &uvbuffer);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &normalbuffer);
        glDeleteVertexArrays(1, &tangentbuffer);
        glDeleteVertexArrays(1, &bitangentbuffer);
    }
    
private:
    void setup(){
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glGenBuffers(1, &uvbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glGenBuffers(1, &normalbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glGenBuffers(1, &tangentbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer);
        glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(glm::vec3), &tangents[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);	
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glGenBuffers(1, &bitangentbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer);
        glBufferData(GL_ARRAY_BUFFER, bitangents.size() * sizeof(glm::vec3), &bitangents[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(4);	
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0] , GL_STATIC_DRAW);
    }

    void computeTangentBasis(
        // inputs
        std::vector<glm::vec3> & vertices,
        std::vector<glm::vec2> & uvs,
        std::vector<glm::vec3> & normals,
        // outputs
        std::vector<glm::vec3> & tangents,
        std::vector<glm::vec3> & bitangents
    ){

        for (unsigned int i=0; i<vertices.size(); i+=3 ){

            // Shortcuts for vertices
            glm::vec3 & v0 = vertices[i+0];
            glm::vec3 & v1 = vertices[i+1];
            glm::vec3 & v2 = vertices[i+2];

            // Shortcuts for UVs
            glm::vec2 & uv0 = uvs[i+0];
            glm::vec2 & uv1 = uvs[i+1];
            glm::vec2 & uv2 = uvs[i+2];

            // Edges of the triangle : postion delta
            glm::vec3 deltaPos1 = v1-v0;
            glm::vec3 deltaPos2 = v2-v0;

            // UV delta
            glm::vec2 deltaUV1 = uv1-uv0;
            glm::vec2 deltaUV2 = uv2-uv0;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y)*r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x)*r;

            // Set the same tangent for all three vertices of the triangle.
            // They will be merged later, in vboindexer.cpp
            tangents.push_back(tangent);
            tangents.push_back(tangent);
            tangents.push_back(tangent);

            // Same thing for binormals
            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);

        }

        // See "Going Further"
        for (unsigned int i=0; i<vertices.size(); i+=1 )
        {
            glm::vec3 & n = normals[i];
            glm::vec3 & t = tangents[i];
            glm::vec3 & b = bitangents[i];
            
            // Gram-Schmidt orthogonalize
            t = glm::normalize(t - n * glm::dot(n, t));
            
            // Calculate handedness
            if (glm::dot(glm::cross(n, t), b) < 0.0f){
                t = t * -1.0f;
            }

        }


    }
};
#endif