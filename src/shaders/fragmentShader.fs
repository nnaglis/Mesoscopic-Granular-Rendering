#version 410 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Fnormal;
in vec3 FragPos;

uniform sampler2D tex;

// light position
uniform vec3 lightDir;

// camera position
uniform vec3 eyePos;


// scattering coefficient
uniform vec3 sigma_s = vec3(0.74, 0.88, 1.01);

// absorption coefficient
uniform vec3 sigma_a = vec3(0.032, 0.17, 0.48);


// anisotropy parameter for the Henyey-Greenstein phase function
uniform float g=0.0;


uniform float n_glass = 1.6;

//PI
const float PI = 3.14159265359;

// roughness
// since im using the same roughness for x and y axis, it is squared
uniform float roughness = 0.05;



// Henyey-Greenstein phase function
float hgPhaseFunction(vec3 lightDir, vec3 viewDir) {
    float cosTheta = dot(lightDir, viewDir);
    return 1.0 / (4.0 * PI) * (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);
}

float Rs(float cosTheta, float n1, float n2) {
    return (n1 * cosTheta - n2 * sqrt(1.0 - (n1 / n2) * (n1 / n2) * (1.0 - cosTheta * cosTheta))) /
           (n1 * cosTheta + n2 * sqrt(1.0 - (n1 / n2) * (n1 / n2) * (1.0 - cosTheta * cosTheta)));
}

float Rp(float cosTheta, float n1, float n2) {
    return (n1 * sqrt(1.0 - (n1 / n2) * (n1 / n2) * (1.0 - cosTheta * cosTheta)) - n2 * cosTheta) /
           (n1 * sqrt(1.0 - (n1 / n2) * (n1 / n2) * (1.0 - cosTheta * cosTheta)) + n2 * cosTheta);
}

// Trowbridge-Reitz GGX Normal Distribution Function
float Distribution_GGX(float NdotH, float roughness)
{
    NdotH = max(NdotH, 0.0);
    float a2 = roughness*roughness;
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

// Smith monodirectional shadowing-masking function (from Walter Torrance 2007 paper)
float Geometry_Smith(float NdotV, float NdotL, float roughness)
{
    NdotV = max(NdotV, 0.0);
    NdotL = max(NdotL, 0.0);
    float a2 = roughness*roughness;
    // G = GGX(V) * GGX(L)
    float GGXV = 2.0 * NdotV / (NdotV + sqrt(a2 + (1.0 - a2) * NdotV * NdotV));
    float GGXL = 2.0 * NdotL / (NdotL + sqrt(a2 + (1.0 - a2) * NdotL * NdotL));
    return GGXV * GGXL;  
}

void main()
{   

    vec3 lightDir = normalize(-lightDir);
    // diffuse reflection
    float Fdr = - 1.440 / (n_glass * n_glass) + 0.710 / n_glass + 0.668 + 0.0636 * n_glass;
    
    // Fresnel reflection
    float A = (1.0 + Fdr) / (1.0 - Fdr);

    // Albedo
    vec3 sigma_t = sigma_s + sigma_a;
    vec3 albedo = sigma_s / sigma_t;

    // Reduced albedo
    vec3 sigma_s_prime = ( 1.0 - g ) * sigma_s;
    vec3 sigma_t_prime = sigma_s_prime + sigma_a;
    vec3 albedo_prime = sigma_s_prime / sigma_t_prime;

    //
    vec3 Rd = (albedo_prime / 2.0) * (1.0 + exp(-4.0/3.0 * A * sqrt(3.0 * (1.0 - albedo_prime)))) * exp(-sqrt(3.0 * (1.0 - albedo_prime)));
    

    // s and p polarizations
    // n1 = 1.0, n2 = 1.6
    float cos_incident = dot(Fnormal, lightDir);
    float Rs_1 = Rs(cos_incident, 1.0, n_glass);
    float Rp_1 = Rp(cos_incident, 1.0, n_glass);
    float Fr_1 = 0.5 * (abs(Rs_1 * Rs_1) + abs(Rp_1 * Rp_1));
    float Ft_1 = 1.0 - Fr_1;
    // Ft_1 = max(Ft_1, 0.0);

    // Ft for out-scattering
    vec3 viewDir = normalize( eyePos - FragPos);
    float cos_refracted = dot(Fnormal, viewDir);
    float sin_refracted = sqrt(1.0 - cos_refracted * cos_refracted);
    // //from glass to air
    float sin_incident = sin_refracted / n_glass;

    float cos_incident_2 = sqrt(1.0 - sin_incident * sin_incident);
    float Rs_2 = Rs(cos_incident_2, n_glass, 1.0);
    float Rp_2 = Rp(cos_incident_2, n_glass, 1.0);
    float Fr_2 = 0.5 * (abs(Rs_2 * Rs_2) + abs(Rp_2 * Rp_2));
    float Ft_2 = 1.0 - Fr_2;

    float F = Ft_1 * Ft_2;
    //single scattering term
    vec3 single_scattering = albedo * F * hgPhaseFunction(normalize(lightDir), viewDir) / ( abs(dot(-Fnormal, normalize(lightDir))) + abs(dot(-Fnormal, viewDir)) );

    vec3 BSSRDF = single_scattering + F*Rd/PI;

    // vec3 baseColor = vec3(1.0, 0.546, 0.371);
    vec3 baseColor = vec3(1.0, 0.803, 0.705);


    //diffuse lighting
    float diffuse = max(dot(Fnormal, lightDir)* 1/PI, 0.0);

    //ambient lighting
    float ambient = 0.2;

    //specular lighting
    float specularStrength = 0.8;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // using the GGX normal distribution function
    float NDF = Distribution_GGX(dot(Fnormal, halfwayDir), roughness);
    float G = Geometry_Smith(dot(Fnormal, viewDir), dot(Fnormal, lightDir), roughness);
    // using Blinn-Phong
    // float specular = pow(max(dot(Fnormal, halfwayDir), 0.0), (2/(roughness*roughness)-2.0))* (1/(PI*roughness*roughness)) ;

    float numerator    = NDF * G * /*F */ F;
    float denominator = 4.0 * max(dot(Fnormal, viewDir), 0.0) * max(dot(Fnormal, lightDir), 0.0) + 0.000001; // + 0.0001 to prevent divide by zero
    float specular = numerator / denominator;

    // vec3 result = Fr_final+ (specular+diffuse+ambient)* vec3(1.0, 0.803, 0.705);
    vec3 result = vec3(specular);
    // vec3 result = vec3(texture(tex, TexCoords).r);
    // float result = texture(tex, TexCoords).r;
    // result += 0.1;
    // vec3 result2 = vec3(result);
    FragColor = vec4(result, 1.0);
}
