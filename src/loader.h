#ifndef LOADER_H
#define LOADER_H

#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <ctime>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "debuggl.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "material.h"
#include "filesystem.h"

class Loader {
public:
    Loader();
    ~Loader();
    
    void loadObj(const char* path, std::vector<Mesh>& meshes, std::vector<Material>& materials);      
    unsigned int loadTexture(char const* path);
    
private:
    void getMeshes(const aiScene* scene, std::vector<Mesh>& meshes);  
    void getMesh(const aiMesh* mesh, std::vector<Mesh>& meshes);
    void getMaterials(const char* path, const aiScene* scene, std::vector<Mesh>& meshes, std::vector<Material>& materials);
    void getMaterial(const char* path, const aiMaterial* material, std::vector<Material>& materials);

};

#endif