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

class Loader {
public:
    Loader();
    ~Loader();
    
    void loadObj(const char* path, 
        std::vector<glm::vec4>& vertices, 
        std::vector<glm::vec2>& uvs,
        std::vector<glm::vec4>& normals,
        std::vector<glm::uvec3>& faces);
        
    unsigned int loadTexture(char const* path);
    
private:

};

#endif