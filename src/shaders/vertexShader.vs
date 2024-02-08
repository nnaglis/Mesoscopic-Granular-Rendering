#version 410 core

layout (location = 0) in vec3 aPos; // position has attribute position 0
layout (location = 1) in vec2 vertexUV; // texture coordinates

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 fragUV;

void main() {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aPos, 1.0);
    fragUV = vertexUV; // pass the texture coordinates to the fragment shader
}
