#version 410 core

// Ouput data
out vec4 FragColor;

in vec3 Fnormal;

void main()
{             
    FragColor = vec4((normalize(Fnormal)), 1.0);
}