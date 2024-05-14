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
uniform vec3 sigma_s = vec3(2.19, 2.62, 3.00);
uniform vec3 sigma_a = vec3(0.0021, 0.0041, 0.0071);
uniform float g = 0.0;
uniform float n_material = 2.4;
uniform float roughness = 0.0;


uniform vec3 reflectance = vec3(1.0);

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
        // // amount of light that is reflected by the material
        // vec3 reflectance;
};

// TESTING VARIABLES
    //ambient lighting
        float ambient = 0.2;


// Henyey-Greenstein phase function
float hgPhaseFunction(vec3 wi, vec3 wo) {
    float cosTheta = dot(wi, wo);
    return 1.0 / (4.0 * PI) * (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);
}

float Rs(float cosI, float cosT, float n1, float n2) {
    float term1 = n1 * cosI - n2 * cosT;
    float term2 = n1 * cosI + n2 * cosT;
    if (term1 == 0.0 || term2 == 0.0) 
        return 0.0;
    return  term1/term2;
}

float Rp(float cosI, float cosT, float n1, float n2) {
    float term1 = n1 * cosT - n2 * cosI;
    float term2 = n1 * cosT + n2 * cosI;
    if (term1 == 0.0 || term2 == 0.0) 
        return 0.0;
    return  term1/term2;
}

// Trowbridge-Reitz GGX Normal Distribution Function
float Distribution_GGX(float NdotH, float roughness)
{
    // NdotH = max(NdotH, 0.0);
    float a2 = roughness*roughness;
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    if (denom == 0.0) return 0.0;
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
    float denom = (1 + lambda(tan2theta_w, roughness));
    if (denom == 0.0) return 0.0;
    return 1 / denom;
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
    float denom = (1 + lambda(tan2theta_wi, roughness) + lambda(tan2theta_wo, roughness));
    if (denom == 0.0) return 0.0;
    return 1 / denom;
}

float D(vec3 w, vec3 h, vec3 normal, float roughness)
{
    return Distribution_GGX(dot(h, normal), roughness);
}

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

float FresnelReflection(float n1, float n2, float cosT, float cosI)
{
    float Rs = Rs(cosI, cosT, n1, n2);
    float Rp = Rp(cosI, cosT, n1, n2);
    float Fr = 0.5 * (abs(Rs * Rs) + abs(Rp * Rp));
    return Fr;
}

vec3 SingleScattering(vec3 albedo, float Fresnel, vec3 normal, vec3 wi, vec3 wo)
{
    vec3 nom = albedo * Fresnel * hgPhaseFunction(normalize(wi), wo);
    float denom = abs(dot(normal, normalize(wi))) + abs(dot(normal, wo));
    if (denom == 0.0) return vec3(0.0);
    // vec3 single_scattering = albedo * Fresnel * hgPhaseFunction(normalize(wi), wo) / ( abs(dot(normal, normalize(wi))) + abs(dot(normal, wo)) );
    return nom/denom;
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
    // material.reflectance = reflectance;

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
            float sin_incident = sqrt(1.0 - cos_incident * cos_incident);
            float sin_refracted = sin_incident / material.n;
            float cos_refracted = sqrt(1.0 - sin_refracted * sin_refracted);

            float Fr_1 = FresnelReflection(1.0, material.n, cos_refracted, max(cos_incident, 0.0));
            float Ft_1 = 1.0 - Fr_1;
            // Ft_1 = max(Ft_1, 0.0);
        // Ft for out-scattering
            
            float cos_refracted_2 = dot(Fnormal, wo);
            float sin_refracted_2 = sqrt(1.0 - cos_refracted_2 * cos_refracted_2);
            //from material to air
                float sin_incident_2 = sin_refracted_2 / material.n;
                float cos_incident_2 = sqrt(1.0 - sin_incident_2 * sin_incident_2);
                float Fr_2 = FresnelReflection(material.n, 1.0, cos_refracted_2, max(cos_incident_2, 0.0));
                Fr_2 = clamp(Fr_2, 0.0, 1.0);
                float Ft_2 = 1.0 - Fr_2;
                // Ft_2 = max(Ft_2, 0.0);
        // full Fresnel term
            float Fresnel = Ft_1 * Ft_2;  

        //single scattering term
        vec3 single_scattering = SingleScattering(material.albedo, Fresnel, Fnormal, wi, wo)*0.0;

        // Full BSSRDF approximation with BRDF
        vec3 BSSRDF = (single_scattering + Fresnel*DiffuseReflectance/PI) * max(cos_incident,0.0);

        //diffuse lighting
            // 1/PI is a normalization factor to ensure that total energy is conserved
        vec3 diffuse = Li / PI * reflectance;

        

        // specular lighting
            vec3 halfwayDir = normalize(wi + wo);
            // Normal Distribution Function
            float D = D(wi, halfwayDir, Fnormal, roughness);
            // Geometry Shadowing Function
            float G = G(wi, wo, Fnormal, roughness);


            float numerator = D * G * /*Fr_1*/ 1.0;
            float denominator = 4.0 * abs(dot(Fnormal, wo))*abs(dot(Fnormal, wi));
            float BRDF = 0.0;
            // Avoid division by zero
            if(denominator != 0.0) {
                BRDF = numerator / denominator * max(cos_incident,0.0);
            }
        
        resultFcolor += (BRDF + BSSRDF) * lightRadiance;
        // resultFcolor += diffuse;
        // resultFcolor += BRDF * lightRadiance;

        
    }
    FragColor = vec4(resultFcolor, 1.0);
}
