#pragma once

#include "Mesh.h"
#include <vector>
#include <glm/glm.hpp>

class InstancedMesh : public Mesh {
public:
    InstancedMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~InstancedMesh();

    // 更新实例数据
    void updateInstanceData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& colors);
    
    // 绘制所有实例
    void drawInstanced(unsigned int instanceCount);

private:
    unsigned int m_instanceVBO_Pos;
    unsigned int m_instanceVBO_Color;
    size_t m_capacityPos;
    size_t m_capacityColor;
};
