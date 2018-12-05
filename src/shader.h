#ifndef SHADER_H
#define SHADER_H

// OpenGL library includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "debuggl.h"

#include <string>

// VBO and VAO descriptors.
enum { kVertexBuffer, kNormalBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kFloorVao, kScreenVao, kNumVaos };

class ShaderProgram
{
public:
    // program ID
    unsigned int program_id;
    ShaderProgram(const char* vertex_shader, const char* fragment_shader, GLuint& buffer_objects);

    void useProgram();

    // functions used to set uniforms
    void setBoolUniform(const std::string &name, bool value) const;
    void setIntUniform(const std::string &name, int value) const;
    void setFloatUniform(const std::string &name, float value) const;
};

#endif
