#version 400 core

uniform vec4 objectColor; //for objectColor

uniform vec4 ambientColor; //ambient color
uniform float ambientIntensity; //ambient light intensity

uniform vec4 lightColor;//  lightColor (influence diffuse & specular)
uniform vec3 lightPosition; // diffuse light position unit vector world space

uniform vec3 specularCameraPosition; //Vector3 for specular direction
uniform float glossiness; //for the power


out vec4 FragColor;
in vec3 fPos;
in vec3 fNor;
in vec2 uv;

void main()
{
    //basic data
    vec3 lightDirection = normalize(lightPosition - fPos);
    float distanceToLight = length(lightPosition - fPos);
    vec3 reflectionVector = normalize(-lightDirection - fNor * 2 * dot(fNor, -lightDirection));
    vec3 toCameraVector = normalize(specularCameraPosition - fPos); // from fragment to camera

    //ambient
    vec4 ambientColorResult = vec4(ambientIntensity * ambientColor.xyz, 1);
    
    //attenuation
    float attenuation = 1.0 / ( 1.0 + 0.5 * distanceToLight + 0.1 * distanceToLight * distanceToLight);

    //diffuse
    float diffuseIntensity = max(dot(lightDirection, normalize(fNor)), 0);// dot product
    vec4 diffuseColorResult = vec4(diffuseIntensity * lightColor.xyz * attenuation, 1);

    //specular
    float specularIntensity = pow(max(dot(reflectionVector, toCameraVector), 0), glossiness); 
    vec4 specularColorResult = vec4(lightColor.xyz * specularIntensity * attenuation, 1);

    //combine
    FragColor = objectColor * (diffuseColorResult + ambientColorResult + specularColorResult);
    FragColor.a = 1;
}