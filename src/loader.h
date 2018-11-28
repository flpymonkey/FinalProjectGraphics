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

class Loader {
public:
    Loader();
    ~Loader();
    
    void loadObj(const char* path, std::vector<Mesh>& meshes);      
    unsigned int loadTexture(char const* path);
    
private:
    void getMeshes(const aiScene* scene, std::vector<Mesh>& meshes);  
    void getMesh(const aiMesh* mesh, std::vector<Mesh>& meshes);

};

#endif