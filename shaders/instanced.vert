#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aInstancePos;   // 实例位置
layout (location = 4) in vec3 aInstanceColor; // 实例颜色

out VS_OUT {
    vec3 vPosition;
    vec3 vNormal;
    vec2 vTexCoord;
    vec3 vColor;
} vs_out;

uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    // 简单位移，无旋转缩放
    vec3 worldPos = aPosition + aInstancePos;
    
    vs_out.vPosition = worldPos;
    vs_out.vNormal = aNormal;
    vs_out.vTexCoord = aTexCoord;
    vs_out.vColor = aInstanceColor;
    
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
