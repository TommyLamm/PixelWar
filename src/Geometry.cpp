// ============================================================================
// Geometry.cpp - 基本几何体生成实现
// 功能: 生成立方体、平面等基本几何体的网格数据
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#include "Geometry.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace Geometry
{
    MeshData createCubeData(float size)
    {
        float s = size * 0.5f;

        // 立方体的24个顶点（每个面4个顶点，共6个面）
        std::vector<Vertex> vertices = {
            // 前面 (Z+)
            Vertex(glm::vec3(-s, -s,  s), glm::vec3(0, 0, 1), glm::vec2(0, 0)),
            Vertex(glm::vec3( s, -s,  s), glm::vec3(0, 0, 1), glm::vec2(1, 0)),
            Vertex(glm::vec3( s,  s,  s), glm::vec3(0, 0, 1), glm::vec2(1, 1)),
            Vertex(glm::vec3(-s,  s,  s), glm::vec3(0, 0, 1), glm::vec2(0, 1)),

            // 后面 (Z-)
            Vertex(glm::vec3( s, -s, -s), glm::vec3(0, 0, -1), glm::vec2(0, 0)),
            Vertex(glm::vec3(-s, -s, -s), glm::vec3(0, 0, -1), glm::vec2(1, 0)),
            Vertex(glm::vec3(-s,  s, -s), glm::vec3(0, 0, -1), glm::vec2(1, 1)),
            Vertex(glm::vec3( s,  s, -s), glm::vec3(0, 0, -1), glm::vec2(0, 1)),

            // 上面 (Y+)
            Vertex(glm::vec3(-s,  s,  s), glm::vec3(0, 1, 0), glm::vec2(0, 0)),
            Vertex(glm::vec3( s,  s,  s), glm::vec3(0, 1, 0), glm::vec2(1, 0)),
            Vertex(glm::vec3( s,  s, -s), glm::vec3(0, 1, 0), glm::vec2(1, 1)),
            Vertex(glm::vec3(-s,  s, -s), glm::vec3(0, 1, 0), glm::vec2(0, 1)),

            // 下面 (Y-)
            Vertex(glm::vec3(-s, -s, -s), glm::vec3(0, -1, 0), glm::vec2(0, 0)),
            Vertex(glm::vec3( s, -s, -s), glm::vec3(0, -1, 0), glm::vec2(1, 0)),
            Vertex(glm::vec3( s, -s,  s), glm::vec3(0, -1, 0), glm::vec2(1, 1)),
            Vertex(glm::vec3(-s, -s,  s), glm::vec3(0, -1, 0), glm::vec2(0, 1)),

            // 右面 (X+)
            Vertex(glm::vec3( s, -s,  s), glm::vec3(1, 0, 0), glm::vec2(0, 0)),
            Vertex(glm::vec3( s, -s, -s), glm::vec3(1, 0, 0), glm::vec2(1, 0)),
            Vertex(glm::vec3( s,  s, -s), glm::vec3(1, 0, 0), glm::vec2(1, 1)),
            Vertex(glm::vec3( s,  s,  s), glm::vec3(1, 0, 0), glm::vec2(0, 1)),

            // 左面 (X-)
            Vertex(glm::vec3(-s, -s, -s), glm::vec3(-1, 0, 0), glm::vec2(0, 0)),
            Vertex(glm::vec3(-s, -s,  s), glm::vec3(-1, 0, 0), glm::vec2(1, 0)),
            Vertex(glm::vec3(-s,  s,  s), glm::vec3(-1, 0, 0), glm::vec2(1, 1)),
            Vertex(glm::vec3(-s,  s, -s), glm::vec3(-1, 0, 0), glm::vec2(0, 1)),
        };

        // 立方体的36个索引（12个三角形，每个三角形3个索引）
        std::vector<GLuint> indices = {
            // 前面
            0, 1, 2, 0, 2, 3,
            // 后面
            4, 5, 6, 4, 6, 7,
            // 上面
            8, 9, 10, 8, 10, 11,
            // 下面
            12, 13, 14, 12, 14, 15,
            // 右面
            16, 17, 18, 16, 18, 19,
            // 左面
            20, 21, 22, 20, 22, 23,
        };

        return { vertices, indices };
    }

    Mesh* createCube(float size)
    {
        MeshData data = createCubeData(size);
        return new Mesh(data.vertices, data.indices);
    }

    Mesh* createPlane(float width, float height, int widthSegments, int heightSegments)
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;

        float xStep = width / widthSegments;
        float zStep = height / heightSegments;

        // 生成顶点
        for (int z = 0; z <= heightSegments; ++z)
        {
            for (int x = 0; x <= widthSegments; ++x)
            {
                float posX = -halfWidth + x * xStep;
                float posZ = -halfHeight + z * zStep;
                float texX = static_cast<float>(x) / widthSegments;
                float texZ = static_cast<float>(z) / heightSegments;

                vertices.emplace_back(
                    glm::vec3(posX, 0.0f, posZ),      // 位置
                    glm::vec3(0.0f, 1.0f, 0.0f),      // 法线指向Y轴正方向
                    glm::vec2(texX, texZ)              // 纹理坐标
                );
            }
        }

        // 生成索引
        for (int z = 0; z < heightSegments; ++z)
        {
            for (int x = 0; x < widthSegments; ++x)
            {
                int row1 = z * (widthSegments + 1);
                int row2 = (z + 1) * (widthSegments + 1);

                // 第一个三角形
                indices.push_back(row1 + x);
                indices.push_back(row2 + x);
                indices.push_back(row1 + x + 1);

                // 第二个三角形
                indices.push_back(row1 + x + 1);
                indices.push_back(row2 + x);
                indices.push_back(row2 + x + 1);
            }
        }

        return new Mesh(vertices, indices);
    }
}

