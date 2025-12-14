#include "InstancedMesh.h"
#include <glad/glad.h>
#include <iostream>

InstancedMesh::InstancedMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : Mesh(vertices, indices), m_instanceVBO_Pos(0), m_instanceVBO_Color(0)
{
    // Mesh 构造函数已经设置了 VAO 和 基础 VBO (Pos, Normal)
    // 现在我们需要添加实例属性
    glBindVertexArray(VAO);

    // 生成实例缓冲
    glGenBuffers(1, &m_instanceVBO_Pos);
    glGenBuffers(1, &m_instanceVBO_Color);

    // 绑定实例位置缓冲 (Layout 3)
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO_Pos);
    // 预分配空间或初始化为空
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1); // 告诉 OpenGL 这个属性每 1 个实例更新一次

    // 绑定实例颜色缓冲 (Layout 4)
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO_Color);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1); // 告诉 OpenGL 这个属性每 1 个实例更新一次

    glBindVertexArray(0);
}

InstancedMesh::~InstancedMesh()
{
    glDeleteBuffers(1, &m_instanceVBO_Pos);
    glDeleteBuffers(1, &m_instanceVBO_Color);
}

void InstancedMesh::updateInstanceData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& colors)
{
    glBindVertexArray(VAO);

    // 更新位置数据
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO_Pos);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);

    // 更新颜色数据
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO_Color);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void InstancedMesh::drawInstanced(unsigned int instanceCount)
{
    glBindVertexArray(VAO);
    // 使用 glDrawElementsInstanced 替代 glDrawElements
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
    glBindVertexArray(0);
}
