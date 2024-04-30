#version 410 core
#define MAX_LIGHTS 2
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Fnormal;
in vec3 FragPos;

uniform sampler2D tex;

// lights
uniform int numLights;
uniform vec3 lightDirections[MAX_LIGHTS];
uniform vec3 lightRadiances[MAX_LIGHTS];

// camera position
uniform vec3 eyePos;

//PI constant
const float PI = 3.14159265359;

// Predefined material properties
uniform vec3 sigma_s = vec3(2.29, 2.39 , 1.97);
uniform vec3 sigma_a = vec3(0.0030, 0.0034, 0.046);
uniform float g = 0.0;
uniform float n_material = 1.3;
uniform float roughness = 0.1;
uniform vec3 reflectance = vec3(1.0 , 1.0, 1.0);

// Material properties
struct MaterialProperties {
        // scattering coefficient
        vec3 sigma_s;
        // absorption coefficient
        vec3 sigma_a;
        // albedo
        vec3 albedo;
        vec3 albedo_prime;
        // anisotropy parameter for the Henyey-Greenstein phase function
        float g;
        // refraction coefficient of the material
        float n;
        // roughness
            // since im using the same roughness for x and y axis, it is a single value
        float roughness;
        // amount of light that is reflected by the material
        vec3 reflectance;
};

// TESTING VARIABLES
    //ambient lighting
        float ambient = 0.2;


// Henyey-Greenstein phase function
float hgPhaseFunction(vec3 wi, vec3 wo) {
    float cosTheta = dot(wi, wo);
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
    // NdotH = max(NdotH, 0.0);
    float a2 = roughness*roughness;
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float lambda(float tan2theta, float roughness)
{
    float lambda = (sqrt(1.0 + roughness * roughness * tan2theta) - 1.0) /2;
    return lambda;
}

float G1(vec3 w, vec3 normal, float roughness)
{
    float cos2theta_w = dot(w, normal) * dot(w, normal);
    float sin2theta_w = 1.0 - cos2theta_w;
    float tan2theta_w = sin2theta_w / cos2theta_w;
    return 1 / (1 + lambda(tan2theta_w, roughness));
}

float G(vec3 wi, vec3 wo, vec3 normal, float roughness)
{
    float costheta_wi = dot(wi, normal);
    float costheta_wo = dot(wo, normal);
    float cos2theta_wi = costheta_wi * costheta_wi;
    float cos2theta_wo = costheta_wo * costheta_wo;
    float sin2theta_wi = 1.0 - cos2theta_wi;
    float sin2theta_wo = 1.0 - cos2theta_wo;
    float tan2theta_wi = sin2theta_wi / cos2theta_wi;
    float tan2theta_wo = sin2theta_wo / cos2theta_wo;
    return 1 / (1 + lambda(tan2theta_wi, roughness) + lambda(tan2theta_wo, roughness));
}

float D(vec3 w, vec3 h, vec3 normal, float roughness)
{
    float costheta_w = abs(dot(w, normal));
    if (costheta_w == 0.0) return 0.0;
    // return G1(w, roughness) / costheta_w * Distribution_GGX(dot(h, normal), roughness) * abs(dot(w, h), 0.0);
    return Distribution_GGX(dot(h, normal), roughness);
}

// float D_PDF(vec3 w, vec3 h, vec3 normal, float roughness)
// {
//     float costheta_w = abs(dot(w, normal));
//     if (costheta_w == 0.0) return 0.0;
//     return G1(w, normal, roughness) / costheta_w * Distribution_GGX(dot(h, normal), roughness) * abs(dot(w, h));
//     // return Distribution_GGX(dot(h, normal), roughness);
// }

vec3 DiffuseReflectance(float n, vec3 sigma_s, vec3 sigma_a, float g)
{
    // Diffuse fresnel reflection
        float Fdr = - 1.440 / (n * n) + 0.710 / n + 0.668 + 0.0636 * n;
    // Fresnel reflection
        float A = (1.0 + Fdr) / (1.0 - Fdr);
    // Albedo
        vec3 sigma_t = sigma_s + sigma_a;
        vec3 albedo = sigma_s / sigma_t;
    // Reduced albedo
        vec3 sigma_s_prime = ( 1.0 - g ) * sigma_s;
        vec3 sigma_t_prime = sigma_s_prime + sigma_a;
        vec3 albedo_prime = sigma_s_prime / sigma_t_prime;
    // Diffuse Reflectance
        vec3 Rd = (albedo_prime / 2.0) * (1.0 + exp(-4.0/3.0 * A * sqrt(3.0 * (1.0 - albedo_prime)))) * exp(-sqrt(3.0 * (1.0 - albedo_prime)));
    return Rd;
}

float FresnelReflection(float n, float cosTheta)
{
    float Rs = Rs(cosTheta, 1.0, n);
    float Rp = Rp(cosTheta, 1.0, n);
    float Fr = 0.5 * (abs(Rs * Rs) + abs(Rp * Rp));
    return Fr;
}

vec3 SingleScattering(vec3 albedo, float Fresnel, vec3 normal, vec3 wi, vec3 wo)
{
    vec3 single_scattering = albedo * Fresnel * hgPhaseFunction(normalize(wi), wo) / ( abs(dot(normal, normalize(wi))) + abs(dot(normal, wo)) );
    return single_scattering;
}

void main()
{   
    // Creating a material instance with the predefined properties
    MaterialProperties material;
    material.sigma_s = sigma_s;
    material.sigma_a = sigma_a;
    material.g = g;
    material.n = n_material;
    material.roughness = roughness;
    material.reflectance = reflectance;

    // Calculating the albedo
    vec3 sigma_t = sigma_s + sigma_a;
    material.albedo = sigma_s / sigma_t;

    // Calculating the reduced albedo
    vec3 sigma_s_prime = ( 1.0 - g ) * sigma_s;
    vec3 sigma_t_prime = sigma_s_prime + sigma_a;
    material.albedo_prime = sigma_s_prime / sigma_t_prime;

    // Calculating the diffuse reflectance
    vec3 DiffuseReflectance = DiffuseReflectance(material.n, material.sigma_s, material.sigma_a, material.g);

    // Calculating Shading for each light

    // Vec3 for the final color
    vec3 resultFcolor = vec3(0.0);

    for (int i = 0; i < (numLights); i++) {
        vec3 lightDir = normalize(lightDirections[i]);
        vec3 lightRadiance = lightRadiances[i];

        // Defining wi and wo
        vec3 wi = normalize(lightDir);
        vec3 wo = normalize( eyePos - FragPos);
        // incident angle
        float cos_incident = dot(Fnormal, wi);

        vec3 Li = lightRadiance * max(cos_incident, 0.0);
    

        // Ft for in-scattering
            
            float Fr_1 = FresnelReflection(material.n, cos_incident);
            float Ft_1 = 1.0 - Fr_1;
            Ft_1 = max(Ft_1, 0.0);
        // Ft for out-scattering
            
            float cos_refracted = dot(Fnormal, wo);
            float sin_refracted = sqrt(1.0 - cos_refracted * cos_refracted);
            //from material to air
                float sin_incident = sin_refracted / material.n;
                float cos_incident_2 = sqrt(1.0 - sin_incident * sin_incident);
                float Fr_2 = FresnelReflection(1.0, cos_incident_2);
                float Ft_2 = 1.0 - Fr_2;
                Ft_2 = max(Ft_2, 0.0);
        // full Fresnel term
            float Fresnel = Ft_1 * Ft_2;  

        //single scattering term
        vec3 single_scattering = SingleScattering(material.albedo, Fresnel, Fnormal, wi, wo);

        // Full BSSRDF approximation with BRDF
        vec3 BSSRDF = single_scattering + Fresnel*DiffuseReflectance/PI;

        //diffuse lighting
            // 1/PI is a normalization factor to ensure that total energy is conserved
        vec3 diffuse = Li / PI * reflectance;

        

        // //specular lighting
        vec3 halfwayDir = normalize(wi + wo);
        // // using the GGX normal distribution function
        // float NdotH = max(dot(Fnormal, halfwayDir), 0.0);
        // float NDF = Distribution_GGX(NdotH, material.roughness);
        // // float G = Geometry_Smith(dot(Fnormal, wo), dot(Fnormal, wi), roughness);
        // float G = Geometry_Smith(wi, wo, roughness);
        // // using Blinn-Phong
        // // float specular = pow(max(dot(Fnormal, halfwayDir), 0.0), (2/(roughness*roughness)-2.0))* (1/(PI*roughness*roughness)) ;

        // // float numerator    = NDF * G * /*max(Fr_1,0.0) */ 1.0;
        // // float denominator = 4.0 * max(dot(Fnormal, wo), 0.0) * max(dot(Fnormal, wi), 0.0);
        
        // Normal Distribution Function
        float D = D(wi, halfwayDir, Fnormal, roughness);
            // PDF and it's function is Not needed
        // float PDF = D_PDF(wi, halfwayDir, Fnormal, roughness);
        // Geometry Shadowing Function
        float G = G(wi, wo, Fnormal, roughness);


        float numerator = D*G  /* * Fresnel */;
        float denominator = 4.0 * abs(dot(Fnormal, wo))*abs(dot(Fnormal, wi));
        float BRDF = 0.0;
        // Avoid division by zero
        if(denominator != 0.0) {
            BRDF = numerator / denominator * max(cos_incident,0.0);
        }
        
        resultFcolor += BRDF*lightRadiance;
        
    }
    FragColor = vec4(resultFcolor, 1.0);
}
