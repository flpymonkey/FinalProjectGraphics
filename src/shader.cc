#include "shader.h"

ShaderProgram::ShaderProgram(const char* vertex_shader, const char* fragment_shader, GLuint& buffer_objects){
  GLuint vertex_shader_id = 0;
  CHECK_GL_ERROR(vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(vertex_shader_id, 1,
      &vertex_shader, nullptr));
	glCompileShader(vertex_shader_id);
	CHECK_GL_SHADER_ERROR(vertex_shader_id);

  GLuint fragment_shader_id = 0;
	CHECK_GL_ERROR(fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(fragment_shader_id, 1,
				&fragment_shader, nullptr));
	glCompileShader(fragment_shader_id);
	CHECK_GL_SHADER_ERROR(fragment_shader_id);

  this->program_id = 0;
  CHECK_GL_ERROR(this->program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(this->program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(this->program_id, fragment_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(this->program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(this->program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(this->program_id, 0, "fragment_color"));
	glLinkProgram(this->program_id);
	CHECK_GL_PROGRAM_ERROR(this->program_id);
}

void ShaderProgram::setIntUniform(const std::string &name, int value){
  CHECK_GL_ERROR(glUseProgram(this->program_id));
  GLint uniform_data_location = 0;
  CHECK_GL_ERROR(uniform_data_location =
    glGetUniformLocation(this->program_id, name));
  glUniform1i(uniform_data_location, value);
}
