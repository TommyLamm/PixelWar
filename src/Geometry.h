// ============================================================================
// Geometry.h - 基本几何体生成
// 功能: 生成立方体、平面等基本几何体的网格数据
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#pragma once

#include "Mesh.h"
#include <glm/glm.hpp>

/**
 * @namespace Geometry
 * @brief 基本几何体生成
 */
namespace Geometry
{
    /**
     * @struct MeshData
     * @brief 简单的网格数据容器 (用于 InstancedMesh 等需要原始数据的情况)
     */
    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
    };

    /**
     * @brief 创建立方体网格数据 (不创建 Mesh 对象)
     * @param size 立方体的边长
     * @return MeshData 包含顶点和索引数据
     */
    MeshData createCubeData(float size = 1.0f);

    /**
     * @brief 创建立方体网格
     * @param size 立方体的边长
     * @return Mesh* 对象 (堆上分配，调用者负责 delete)
     * @note 为了兼容旧代码，这里返回 Mesh 对象 (按值返回) 或 Mesh* ?
     * 根据之前的错误日志 "error C2440: “初始化”: 无法从“Mesh”转换为“Mesh *”"
     * 说明之前的 createCube 返回的是 Mesh (值)。
     * 但现在的 main.cpp 里是 `g_cubeMesh = Geometry::createCube(1.0f);` 其中 g_cubeMesh 是 Mesh*。
     * 所以我们需要统一接口。
     * 建议 createCube 返回 Mesh* (指针)，或者在 main.cpp 里用 new Mesh(createCubeData(...))。
     * 
     * 让我们修改 createCube 返回 Mesh*，这样最符合 C++ 面向对象的使用习惯 (工厂模式)。
     * 但之前的代码是 Mesh createCube(...) (按值返回)。
     * 如果修改这里，会破坏 Mesh.h 的语义 (Mesh 是资源管理类，按值返回会导致资源转移或拷贝)。
     * Mesh 禁用了拷贝，但允许移动。
     * 
     * 最佳方案：让 createCube 返回 MeshData，然后 Mesh 构造函数接受 MeshData。
     * 或者让 createCube 返回 Mesh*。
     */
    Mesh* createCube(float size = 1.0f); // 修改为返回指针

    /**
     * @brief 创建平面网格（作为地面）
     * ...
     */
    Mesh* createPlane(float width = 10.0f, float height = 10.0f,
                     int widthSegments = 10, int heightSegments = 10); // 修改为返回指针
}

