#version 460
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_colour;
layout(location = 2) in vec2 vertex_texCoord;

// uniform mat4 MVP;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 color;
out vec2 TexCoord;

void main() {
  gl_Position = projection * view * model * vec4(vertex_position, 1.0);
  color = vertex_colour;
  TexCoord = vertex_texCoord;
}