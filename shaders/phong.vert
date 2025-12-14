#version 330 core

// 输入顶点属性
layout (location = 0) in vec3 aPosition;   // 顶点位置
layout (location = 1) in vec3 aNormal;     // 顶点法线
layout (location = 2) in vec2 aTexCoord;   // 纹理坐标

// 与片段着色器传递的数据
out VS_OUT {
    vec3 vPosition;      // 世界空间中的顶点位置
    vec3 vNormal;        // 世界空间中的法线
    vec2 vTexCoord;      // 纹理坐标
} vs_out;

// uniform变量
uniform mat4 uModel;           // 模型矩阵
uniform mat4 uView;            // 视图矩阵
uniform mat4 uProjection;      // 投影矩阵
uniform mat3 uNormalMatrix;    // 法线矩阵（模型视图矩阵的逆转置）

void main()
{
    // 将顶点位置变换到世界空间
    vs_out.vPosition = vec3(uModel * vec4(aPosition, 1.0));
    
    // 使用法线矩阵变换法线到世界空间
    vs_out.vNormal = normalize(uNormalMatrix * aNormal);
    
    // 传递纹理坐标
    vs_out.vTexCoord = aTexCoord;
    
    // 计算最终的顶点位置（投影空间）
    gl_Position = uProjection * uView * vec4(vs_out.vPosition, 1.0);
}
