#version 330 core

in VS_OUT {
    vec3 vPosition;
    vec3 vNormal;
    vec2 vTexCoord;
    vec3 vColor;
} fs_in;

out vec4 FragColor;

uniform vec3 uMaterial_Ambient;
uniform vec3 uMaterial_Specular;
uniform float uMaterial_Shininess;

uniform vec3 uLight_Position;
uniform vec3 uLight_Ambient;
uniform vec3 uLight_Diffuse;
uniform vec3 uLight_Specular;
uniform vec3 uCameraPos;

void main()
{
    vec3 norm = normalize(fs_in.vNormal);
    vec3 lightDir = normalize(uLight_Position - fs_in.vPosition);
    vec3 viewDir = normalize(uCameraPos - fs_in.vPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    // 使用实例颜色作为基色
    vec3 ambient = uLight_Ambient * uMaterial_Ambient * fs_in.vColor;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uLight_Diffuse * fs_in.vColor * diff;
    
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial_Shininess);
    vec3 specular = uLight_Specular * uMaterial_Specular * spec;
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
