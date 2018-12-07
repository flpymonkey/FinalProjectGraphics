R"zzz(#version 330 core

// Input vertex data, different for all executions of this shader.
in vec4 vertex_position;

// Values that stay constant for the whole mesh.
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){

    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  projection * view * model * vertex_position;

}
)zzz"
