// ============================================================================
// Mesh.h - 网格几何体类
// 功能: 管理VAO、VBO、EBO，提供渲染接口
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstddef>

/**
 * @struct Vertex
 * @brief 顶点数据结构
 * @details 
 *   - position: 顶点位置 (x, y, z)
 *   - normal: 顶点法线 (nx, ny, nz)
 *   - texCoord: 纹理坐标 (u, v)
 */
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

    Vertex() : position(0.0f), normal(0.0f), texCoord(0.0f) {}
    
    Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& tex)
        : position(pos), normal(norm), texCoord(tex) {}
};

/**
 * @class Mesh
 * @brief 网格几何体类
 * @details 
 *   - 管理VAO、VBO、EBO
 *   - 存储顶点数据和索引
 *   - 提供渲染接口
 *   - 自动释放GPU资源
 */
class Mesh
{
public:
    std::vector<Vertex> vertices;      // 顶点数据
    std::vector<GLuint> indices;       // 索引数据
    
    // VAO/VBO/EBO的OpenGL ID
    GLuint VAO, VBO, EBO;

    /**
     * @brief 构造函数
     * @param vertices 顶点数据列表
     * @param indices 索引数据列表
     */
    Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);

    /**
     * @brief 析构函数 - 释放GPU资源
     */
    ~Mesh();

    /**
     * @brief 禁用拷贝构造和拷贝赋值
     */
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    /**
     * @brief 允许移动构造和移动赋值
     */
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    /**
     * @brief 渲染网格
     */
    void draw() const;

    /**
     * @brief 手动释放资源
     */
    void cleanup();

    /**
     * @brief 获取网格的顶点数量
     */
    GLsizei getVertexCount() const { return static_cast<GLsizei>(vertices.size()); }

    /**
     * @brief 获取网格的索引数量
     */
    GLsizei getIndexCount() const { return static_cast<GLsizei>(indices.size()); }

private:
    /**
     * @brief 初始化VAO、VBO、EBO
     */
    void setupMesh();
};

