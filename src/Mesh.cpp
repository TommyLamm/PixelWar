// ============================================================================
// Mesh.cpp - 网格几何体类实现
// 功能: 管理VAO、VBO、EBO，提供渲染接口
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#include "Mesh.h"
#include <iostream>
#include <cstddef>  // for offsetof

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
    : vertices(vertices), indices(indices), VAO(0), VBO(0), EBO(0)
{
    setupMesh();
    std::cout << "[Mesh] 网格加载成功! 顶点数: " << vertices.size() 
              << ", 索引数: " << indices.size() << std::endl;
}

Mesh::~Mesh()
{
    cleanup();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertices(std::move(other.vertices)), indices(std::move(other.indices)),
      VAO(other.VAO), VBO(other.VBO), EBO(other.EBO)
{
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        cleanup();
        vertices = std::move(other.vertices);
        indices = std::move(other.indices);
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
    }
    return *this;
}

void Mesh::setupMesh()
{
    // 创建VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // 创建VBO并填充顶点数据
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // 创建EBO并填充索引数据
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // 设置顶点属性指针
    // 位置属性 (layout location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // 法线属性 (layout location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // 纹理坐标属性 (layout location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    // 解绑VAO、VBO、EBO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::draw() const
{
    if (VAO == 0)
    {
        std::cerr << "[Mesh Error] 尝试渲染未初始化的网格!" << std::endl;
        return;
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, getIndexCount(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::cleanup()
{
    if (EBO != 0)
    {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
}

