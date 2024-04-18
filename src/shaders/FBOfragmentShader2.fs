#version 410 core

// Ouput data
out vec4 FragColor;

void main()
{             
    // Not really needed, OpenGL does it anyway
    // fragmentdepth = gl_FragCoord.z;
    vec3 result = vec3(1.0);
    FragColor = vec4(result, 1.0);
    
}