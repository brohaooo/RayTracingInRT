#version 430 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 Normal; // not used currently in this shader
in vec3 FragPos;

uniform sampler2D texture1;
uniform vec4 color; // the base color of the object, it also contains the opacity
uniform bool useTexture = false;
uniform int MaterialType = 0; //  0: lambertian, 1: metal, 2: dielectric, 3: emissive(light source), -1: unknown

uniform samplerCube skyboxTexture; // Cubemap纹理

layout(std140, binding = 0) uniform RenderInfo{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
};

uniform float refractionRatio = 0.3; // aka η aka eta (index of refraction)
uniform float averageSlope = 0.5;  // aka m (roughness)


// light properties
uniform int numOfLights = 1; // not support multiple lights currently, but it won't to too hard to implement
uniform vec3 lightPos = vec3(1.5,0.45,0); // a point light
uniform vec3 lightColor = vec3(2,2,2); 
uniform vec3 ambientLightColor = vec3(0.4,0.4,0.5); // environment light, usually set to sky color


const float PI = 3.14159265359;
const float epsilon = 0.00001;

// attenuation for the point light
uniform float Constant = 1.0;
uniform float Linear = 0.8;
uniform float Quadratic = 0.56;


void main()
{
    vec4 texColor;

    if(useTexture){
        texColor = texture(texture1, TexCoords);
    }
        
    else{
        texColor = vec4(1.0, 1.0, 1.0, 1.0);
    }

    if(MaterialType == 3){// light source, no shading
        FragColor = texColor * vec4(color.xyz, color.w);
        return;
    }
        
    vec3 finalColor, ambientColor, diffuseColor, specularColor;

    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float NdotH = dot(normal, halfwayDir);
    float NdotL = dot(normal, lightDir);
    float NdotV = dot(normal, viewDir);
    float VdotH = dot(viewDir, halfwayDir);

    float Distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (Constant + Linear * Distance + Quadratic * Distance * Distance);


    vec4 baseColor = texColor * color;

    // ambient
    ambientColor = baseColor.xyz * ambientLightColor;
    // diffuse
    float diff = max(NdotL, 0.0);
    diffuseColor =  baseColor.xyz * lightColor * diff;
    diffuseColor *= attenuation;
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
    G = max(G, 0.0);

    // combine
    float FDG = D * F * G;

    specularColor = ( FDG / (PI * max(NdotV,epsilon)) )* lightColor *  baseColor.xyz;
    specularColor *= attenuation;

    if (MaterialType == 1) // metal
    {
        finalColor = ambientColor + diffuseColor + specularColor;
        // reflection to sample the environment map
        vec3 reflectionDir = reflect(-viewDir, normal);
        float LodLevel = averageSlope * 15.0;
        vec3 reflectedColor = textureLod(skyboxTexture, reflectionDir, LodLevel).rgb * baseColor.xyz;
        finalColor = finalColor + reflectedColor;
    }
    else if (MaterialType == 2) // dielectric
    {
        finalColor = ambientColor;
        // reflection to sample the environment map
        vec3 reflectionDir = reflect(-viewDir, normal);
        vec3 reflectedColor = textureLod(skyboxTexture, reflectionDir, 0).rgb * vec3(1.0,1.0,1.0);
        // refraction to sample the environment map
        vec3 refractionDir = refract(-viewDir, normal, refractionRatio);
        vec3 refractedColor = textureLod(skyboxTexture, refractionDir, 0).rgb * vec3(1.0,1.0,1.0);
        // mix the two colors based on the reflectance
        vec3 finalReflectedColor = mix(reflectedColor + diffuseColor + specularColor, refractedColor, min(F, 0.8));
        finalReflectedColor = max(finalReflectedColor, vec3(0.0,0.0,0.0));
        finalColor = finalColor + finalReflectedColor;
    }
    else if (MaterialType == 0) // lambertian
    {
        finalColor = ambientColor + diffuseColor;
    }
    else{
        finalColor = vec3(1.0,0.0,0.0);
    }


    FragColor = vec4(finalColor, baseColor.w);
}
