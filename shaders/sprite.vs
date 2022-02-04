#version 330 core
layout (location = 0) in vec4 vertex;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 projection;
out vec4 frag_position;

void main()
{
    TexCoords = vertex.zw;
    gl_Position = projection * model * vec4(vertex.xy, 0.0f, 1.0f);
    frag_position = model * vec4(vertex.xy, 0.0f, 1.0f);
}