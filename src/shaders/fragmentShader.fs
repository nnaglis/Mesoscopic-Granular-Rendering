#version 410 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Fnormal;

uniform sampler2D tex;

// light position
uniform vec3 lightPos;

// camera position
uniform vec3 eyePos;

void main()
{   
    // base color mimicking the color set up in blender 
    vec3 baseColor = vec3(1.0, 0.546, 0.371);

    //diffuse lighting
    float diffuse = max(dot(Fnormal, lightPos), 0.0);
    //vec3 diffuse = diff * baseColor;

    //ambient lighting
    float ambient = 0.1;
    //vec3 ambient = ambientStrength * baseColor;

    //specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(eyePos - vec3(0.0, 0.0, 0.0));
    vec3 reflectDir = reflect(-lightPos, Fnormal);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    //vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);

    
   vec3 result = (ambient + diffuse + specular) * baseColor;
   FragColor = vec4(result, 1.0);
   //FragColor = vec4( 0*ambient, 255*diffuse, 255*specular, 1.0);
//    FragColor = vec4 ( lightPos.x, lightPos.y, lightPos.z, 1.0);
}

//incident flux (amount of light on the surface)
    // float cosTheta = max(dot(Normal, lightDir), 0.0)
    // float flux =  LightRGB * costheta * diffusecoeff

// reflectance?