#version 410 core
#define MAX_LIGHTS 2
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Fnormal;
in vec3 FragPos;



// texture sampler array for number of lights
uniform sampler2D normalTextures[MAX_LIGHTS]; 
uniform sampler2D vertexTextures[MAX_LIGHTS]; 

uniform mat4 lightSpaceMatrices[MAX_LIGHTS];

// lights
uniform int numLights;
uniform vec3 lightDirections[MAX_LIGHTS];
uniform vec3 lightRadiances[MAX_LIGHTS];

// camera position
uniform vec3 eyePos;

//PI constant
const float PI = 3.14159265359;

// Predefined material properties
uniform vec3 sigma_s_prime = vec3(0.1);
uniform vec3 sigma_a = vec3(0.9);
uniform float g = 0.0;
uniform float n_material = 1.0;
uniform float roughness = 0.00;
uniform float thickness_scale = 3.0;

// plane near and far
uniform float nearPlane;
uniform float farPlane;

// Screen resolution
uniform vec2 resolution;


uniform vec3 reflectance = vec3(0.5);

// Material properties
struct MaterialProperties {
        // scattering coefficient
        vec3 sigma_s_prime;
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
        vec3 sigma_t_prime;
};

// Henyey-Greenstein phase function
float hgPhaseFunction(vec3 wi, vec3 wo, float g) {
    float cosTheta = dot(wi, wo);
    float numerator = 1.0 - g * g;
    float denominator = pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);
    return (1.0 / (4.0 * PI)) * (numerator / denominator);
    // return cosTheta;
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
    float lambda = (sqrt(1.0 + roughness * roughness * tan2theta) - 1.0) / 2.0;
    return lambda;
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
    float denom = (1.0 + lambda(tan2theta_wi, roughness) + lambda(tan2theta_wo, roughness));
    if (denom == 0.0) return 0.0;
    return 1.0 / denom;
}

float D(vec3 w, vec3 h, vec3 normal, float roughness)
{
    return Distribution_GGX(dot(h, normal), roughness);
}

float A(float n)
{
    float C_1 = 0.0;
    float C_2 = 0.0;
    if (n >= 1.0)
        {
            float two_C_1 = -9.23372 + 22.2272 * n - 20.9292 * n * n + 10.2291 * n * n * n - 2.54396 * n * n * n * n + 0.254913 * n * n * n * n * n;
            float three_C_2 = -1641.1 + 135.926 / (n * n * n) - 656.175 / (n * n) + 1376.53 / n + 1213.67 * n - 568.556 * n * n + 164.798 * n * n * n - 27.0181 * n * n * n * n + 1.91826 * n * n * n * n * n; 
            C_1 = two_C_1 / 2.0;
            C_2 = three_C_2 / 3.0;
        }
    if (n < 1.0)
        {
            float two_C_1 = 0.919317 - 3.4793 * n + 6.75335 * n * n - 7.80989 * n * n * n + 4.98554 * n * n * n * n - 1.36881 * n * n * n * n * n;
            float three_C_2 = 0.828421 - 2.62051 * n + 3.36231 * n * n - 1.95284 * n * n * n + 0.236494 * n * n * n * n + 0.145787 * n * n * n * n * n;
            C_1 = two_C_1 / 2.0;
            C_2 = three_C_2 / 3.0;
        }
    float C_e = (1.0/ 2.0) * (1.0 - C_2);
    float C_phi = (1.0 / 4.0) * (1.0 - C_1);
    float nom =  1.0 - C_e;
    float denom = 2.0 * C_phi;
    if (denom == 0.0) return 0.0;
    return nom/denom;
}


vec3 DiffuseReflectance(float n, vec3 sigma_s_prime, vec3 sigma_a)
{
    // Diffuse fresnel reflection
        // float Fdr = - 1.440 / (n * n) + 0.710 / n + 0.668 + 0.0636 * n;
        // float A = (1.0 + Fdr) / (1.0 - Fdr);
        float A = A(n);
    // Reduced albedo
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
    vec3 nom = albedo * Fresnel * hgPhaseFunction(wi, wo, g);
    float denom = abs(dot(normal, wi)) + abs(dot(normal, wo));
    if (denom == 0.0) return vec3(0.0);
    return nom/denom;
}


// Single scattering function
vec3 SingleScattering2(vec3 wi, vec3 wo, vec3 Fnormal, float Fresnel, vec3 sigma_t, float d_r, vec3 albedo, float g) {
    // Phase function
    float phase = hgPhaseFunction(wi, wo, g);
    // Attenuation
    vec3 attenuation = exp(-sigma_t * (d_r));
    float cosTheta_o = dot(Fnormal, wo);
    // float cosTheta_o = 1.0;

    // Single scattering response
    vec3 scattering = albedo * attenuation / (d_r * d_r) * phase  * Fresnel * cosTheta_o ;

    return scattering;
    // return vec3(phase);
}

float LinearizeDepth(float depth, float nearPlane, float farPlane) {
    float z = depth * 2.0 - 1.0; // Transform depth to NDC [-1, 1]
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

vec3 BSSRDF_distance(float r, vec3 albedo_prime, vec3 sigma_a, vec3 sigma_t_prime, float g, float A)
{
    vec3 D = 1.0 / (3.0 * sigma_t_prime);
    vec3 sigma_tr = sqrt(sigma_a / D);
    vec3 z_r = 1.0 / sigma_t_prime;
    vec3 z_v = z_r + 4.0 * A * D;
    vec3 d_r = sqrt(z_r * z_r + r * r);
    vec3 d_v = sqrt(z_v * z_v + r * r);

    vec3 real_source = (sigma_tr * d_r + 1.0) / (d_r * d_r * d_r * sigma_t_prime) * exp(-sigma_tr * d_r);
    vec3 virt_source = z_v * (1.0 + sigma_tr * d_v) / (d_v * d_v * d_v * sigma_t_prime) * exp(-sigma_tr * d_v);

    return albedo_prime / (4.0 * PI) * (real_source + virt_source);
}

void main()
{   
    vec3 sigma_s = sigma_s_prime / (1.0 - g);
    vec3 Fnormal = normalize(Fnormal);
    // Creating a material instance with the predefined properties
    MaterialProperties material;
    material.sigma_s_prime = sigma_s * (1.0 - g);
    material.sigma_a = sigma_a;
    material.g = g;
    material.n = n_material;
    material.roughness = roughness;
    // material.reflectance = reflectance;

    // Calculating the albedo
    // vec3 sigma_s = sigma_s_prime / (1.0 - g);
    vec3 sigma_t = sigma_s + sigma_a;
    material.albedo = sigma_s / sigma_t;

    // Calculating the reduced albedo
    // vec3 sigma_s_prime = ( 1.0 - g ) * sigma_s;
    material.sigma_t_prime = material.sigma_s_prime + material.sigma_a;
    material.albedo_prime = material.sigma_s_prime / material.sigma_t_prime;

    // Calculating the diffuse reflectance
    vec3 DiffuseReflectance = DiffuseReflectance(material.n, material.sigma_s_prime, material.sigma_a);

    // Calculating Shading for each light

    // Vec3 for the final color
    vec3 resultFcolor = vec3(0.0);
 
    for (int i = 0; i < (numLights-1); i++) {

        // calculating thickness of the material
            vec4 fragPosLightSpace = lightSpaceMatrices[i] * vec4(FragPos, 1.0);
            vec3 projCoords = fragPosLightSpace.xyz; /*/ fragPosLightSpace.w; //if perspective division is needed*/
            projCoords = projCoords * 0.5 + 0.5; 

            int sample_size = 1;
            int sample_step = 40;

            int sample_start_x;
            int sample_end_x;
            int sample_start_y;
            int sample_end_y;

            //TODO adding bias to projcoords.xy components
            float bias = 0.001;
            if (projCoords.x < 0.5) {
                projCoords.x += bias;
                sample_start_x = 0;
                sample_end_x = sample_size;
            }
            else{
                projCoords.x -= bias;
                sample_start_x = -sample_size;
                sample_end_x = 0;
            }
            if (projCoords.y < 0.5) {
                projCoords.y += bias;
                sample_start_y = 0;
                sample_end_y = sample_size;
            }
            else{
                projCoords.y -= bias;
                sample_start_y = -sample_size;
                sample_end_y = 0;
            }

        vec3 lightDir = normalize(lightDirections[i]);
        vec3 lightRadiance = lightRadiances[i];

        // Defining wi and wo
        vec3 wi = normalize(lightDir);
        vec3 wo = normalize( eyePos - FragPos);

        vec3 Lo = vec3(0.0);


        vec2 pixel = vec2 (1.0 / resolution.x, 1.0 / resolution.y);
        vec3 frontPos = texture(vertexTextures[i], projCoords.xy).xyz;

        vec3 incidentNormal = texture(normalTextures[i], projCoords.xy).xyz;

        float thickness = length((FragPos - frontPos)*thickness_scale);
        // thickness = 2.4*thickness_scale + thickness/2.0;
        float r = 2.4 * thickness_scale;
        //distance to centroid of a hemisphere of the incident area
        if (dot(Fnormal, wi) <= 0.0)
        {
            thickness = thickness/2.0 + 3.0*r/8.0;
        }
    
            float cos_incident = dot(incidentNormal, wi);

            // find Fresnel term for in-scattering n1 to n2
            float sin_incident = sqrt(1.0 - cos_incident * cos_incident);
            float sin_refracted = sin_incident / material.n;
            float cos_refracted = sqrt(1.0 - sin_refracted * sin_refracted);
            float Fr_1 = FresnelReflection(1.0, material.n, max(cos_refracted,0.0), max(cos_incident, 0.0));
            float Ft_1 = 1.0 - Fr_1;

            // find Fresnel term for out-scattering n2 to n1
            float cos_refracted_2 = dot(Fnormal, wo);
            float sin_refracted_2 = sqrt(1.0 - cos_refracted_2 * cos_refracted_2);
            float sin_incident_2 = sin_refracted_2 / material.n;
            float cos_incident_2 = sqrt(1.0 - sin_incident_2 * sin_incident_2);
            float Fr_2 = FresnelReflection(material.n, 1.0, max(cos_refracted_2,0.0), max(cos_incident_2, 0.0));
            float Ft_2 = 1.0 - Fr_2;
            
            // full Fresnel term
            float Fresnel = Ft_1 * Ft_2;

            // ORIGINAL
            if (dot (Fnormal, wi) <= 0.0)
            {
                Lo += 1.0/PI * BSSRDF_distance(thickness, material.albedo_prime, material.sigma_a, material.sigma_t_prime, g, A(material.n)) * Fresnel * dot(incidentNormal, wi);
            }

        vec3 single_scattering = vec3(0.0);
        vec3 model_1 = vec3(0.0);
        if (dot(Fnormal, wi) >= 0.0)
        {
            single_scattering = SingleScattering(material.albedo, Fresnel, Fnormal, wi, wo) * max(dot(Fnormal, wi), 0.0);
            model_1 = (Fresnel*DiffuseReflectance/PI+ single_scattering) * max(dot(Fnormal, wi), 0.02) ;
        }
        


        float area = 0.000;
        if (dot(Fnormal, wi) < 0.00)
        {
            area =  PI * r * r;
        }
        resultFcolor += ((Lo*area)+model_1)*lightRadiance;
    }
    FragColor = vec4(resultFcolor, 1.0);
}
