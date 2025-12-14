#version 330 core

// 来自顶点着色器的数据
in VS_OUT {
    vec3 vPosition;
    vec3 vNormal;
    vec2 vTexCoord;
} fs_in;

// 输出片段颜色
out vec4 FragColor;

// 材质属性
uniform vec3 uMaterial_Ambient;      // 环境光反射系数
uniform vec3 uMaterial_Diffuse;      // 漫反射系数
uniform vec3 uMaterial_Specular;     // 镜面反射系数
uniform float uMaterial_Shininess;   // 镜面反射指数

// 光源属性
uniform vec3 uLight_Position;        // 光源位置（世界空间）
uniform vec3 uLight_Ambient;         // 环境光照强度
uniform vec3 uLight_Diffuse;         // 漫反射光照强度
uniform vec3 uLight_Specular;        // 镜面反射光照强度

// 相机/观察者位置
uniform vec3 uCameraPos;

vec3 calculatePhongLighting()
{
    // 标准化输入向量
    vec3 norm = normalize(fs_in.vNormal);
    vec3 lightDir = normalize(uLight_Position - fs_in.vPosition);
    vec3 viewDir = normalize(uCameraPos - fs_in.vPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    // 1. 环境光分量
    vec3 ambient = uLight_Ambient * uMaterial_Ambient;
    
    // 2. 漫反射分量
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uLight_Diffuse * uMaterial_Diffuse * diff;
    
    // 3. 镜面反射分量
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial_Shininess);
    vec3 specular = uLight_Specular * uMaterial_Specular * spec;
    
    // 合并所有光照分量
    return (ambient + diffuse + specular);
}

void main()
{
    // 计算Phong光照
    vec3 result = calculatePhongLighting();
    FragColor = vec4(result, 1.0);
}
