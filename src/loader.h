#ifndef LOADER_H
#define LOADER_H

#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

class Loader {
public:
    Loader();
    ~Loader();
    
    void loadObj(const char* path, 
        std::vector<glm::vec4>& vertices, 
        std::vector<glm::vec4>& normals,
        std::vector<glm::uvec3>& faces);
        
    //unsigned int loadTexture(char const* path);
    
private:

};

#endif