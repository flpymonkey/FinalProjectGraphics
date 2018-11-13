R"zzz(#version 330 core
in vec2 vertex_position;
in vec2 aTexCoords;
out vec2 TexCoords;
void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(vertex_position.x, vertex_position.y, 0.0, 1.0);
}
)zzz"
