#ifndef SHADER_H
#define SHADER_H

// OpenGL library includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

class ShaderProgram
{
public:
    // program ID
    unsigned int program_id;
    ShaderProgram(const char* vertex_shader, const char* fragment_shader, GLuint& buffer_objects, int& vao, int& descriptor);

    void useProgram();

    // functions used to set uniforms
    void setBoolUniform(const std::string &name, bool value) const;
    void setIntUniform(const std::string &name, int value) const;
    void setFloatUniform(const std::string &name, float value) const;
};

#endif
