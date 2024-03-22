#version 430 core

out vec4 FragColor;
in vec3 Normal; 
in vec3 FragPos; 

// light properties
const vec3 lightPos = vec3(4,2,0); // a point light
const vec3 lightColor = vec3(1.8,1.8,1.8); 
const vec3 ambientLightColor = vec3(0.8,0.8,0.8); // environment light, usually set to sky color

layout(std140, binding = 0) uniform RenderInfo{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
    // material properties
    float refractionRatio; // aka η aka eta (index of refraction)
    float averageSlope;  // aka m (roughness)
    int renderMode; // 0: full lighting, 1: fresnel only, 2: distribution only, 3: geometric only
    vec3 ambientK; // ka, k is the reflection coefficient
    vec3 diffuseK; // kd
    vec3 specularK; // ks
};





const float PI = 3.14159265359;
const float epsilon = 0.00001;

void main() {
    vec3 finalColor, ambientColor, diffuseColor, specularColor;

    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float NdotH = dot(normal, halfwayDir);
    float NdotL = dot(normal, lightDir);
    float NdotV = dot(normal, viewDir);
    float VdotH = dot(viewDir, halfwayDir);


    // ambient
    ambientColor = ambientK * ambientLightColor;
    // diffuse
    float diff = max(NdotL, 0.0);
    diffuseColor = diffuseK * lightColor * diff;
    // specular -- Cook-Torrance BRDF
    float F, D, G;
    // F: Fresnel term
    // we use Schlick's approximation
    float F0 = pow((1.0 - refractionRatio) / (1.0 + refractionRatio + epsilon), 2.0);
    F = F0 + (1.0 - F0) * pow(1.0 - dot(lightDir, viewDir), 5.0);
    // D: roughness term
    // we use the Beckmann distribution function
    float m = averageSlope;
    float m2 = m * m;
    float NdotH2 = NdotH * NdotH;
    float tanAlpha2 = (1.0 - NdotH2) / (NdotH2+epsilon);
    float numerator = exp(-tanAlpha2 / (m2+epsilon));
    float denominator = 4.0 * m2 * NdotH2 * NdotH2;
    D = numerator / (denominator+epsilon);
    
    // G: geometry term
    // we use the Cook-Torrance geometry function
    float Masking = 2.0 * NdotH * NdotV / (VdotH+epsilon);
    float Shadowing = 2.0 * NdotH * NdotL / (VdotH+epsilon);
    G = min(1.0, min(Masking, Shadowing));

    // combine
    float FDG;
    if (renderMode == 0){
        FDG = D * F * G;
    }
    else if (renderMode == 1){
        FDG = F;
    }
    else if (renderMode == 2){
        FDG = D;
    }
    else if (renderMode == 3){
        FDG = G;
    }
    specularColor = ( FDG / (PI * NdotV + epsilon) )* lightColor * specularK;

    finalColor = ambientColor + diffuseColor + specularColor;



    FragColor = vec4(finalColor, 1.0);
    //FragColor = vec4(refractionRatio, averageSlope, 0.0, 1.0);
}

