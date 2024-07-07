#version 410 core
out vec4 FragColor;

// Near and Far clipping planes
uniform float nearPlane = 0.1;
uniform float farPlane = 100.0;

//PI
const float PI = 3.14159265359;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{   
    float depth = (LinearizeDepth(gl_FragCoord.z) - nearPlane) / (farPlane - nearPlane);
    //FragDepth = depth;
    float result = 0.2f;
    FragColor = vec4(depth, depth, depth, 1.0);
}
