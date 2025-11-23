#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fragTexCoords;

out vec4 fColor;

#define MAX_LIGHTS 200// <-- KEEP THIS (defines array size)

struct PointLight {
    vec3 position;
    vec3 color;

    float constant;
    float linear;
    float quadratic;
};

uniform PointLight lights[MAX_LIGHTS];

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform int numLights;// <-- ADD THIS

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

void computeLightComponents()
{
    vec3 cameraPosEye = vec3(0.0f);
    vec3 normal = normalize(fNormal);
    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);

    vec3 texColor = texture(diffuseTexture, fragTexCoords).rgb;
    float specularMap = texture(specularTexture, fragTexCoords).r;


    ambient = vec3(0.0f);
    diffuse = vec3(0.0f);
    specular = vec3(0.0f);

    // VVV CHANGE THIS LOOP VVV
    for (int i = 0; i < numLights; i++)// Use the new uniform
    {
        // ^^^ CHANGE THIS LOOP ^^^
        vec3 lightDirN = normalize(lights[i].position - fPosEye.xyz);
        float dist = length(lights[i].position - fPosEye.xyz);

        float att = 1.0f / (lights[i].constant + lights[i].linear * dist + lights[i].quadratic * (dist * dist));

        ambient += (att * ambientStrength * lights[i].color) * texColor;

        float diffCoeff = max(dot(normal, lightDirN), 0.0f);
        diffuse += (att * diffCoeff * lights[i].color) * texColor;

        vec3 reflection = reflect(-lightDirN, normal);
        float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);

        specular += att * specularStrength * specCoeff * specularMap * lights[i].color;
    }
}

void main()
{
    computeLightComponents();

    vec3 baseColor = vec3(0.9f, 0.35f, 0.25f);// orange

    ambient *= baseColor;
    diffuse *= baseColor;
    specular *= baseColor;

    vec3 color = min((ambient + diffuse) + specular, 1.0f);

    fColor = vec4(color, 1.0f);
}