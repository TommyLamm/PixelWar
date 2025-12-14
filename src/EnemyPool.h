#pragma once

#include "Enemy.h"
#include <glm/glm.hpp>
#include <vector>
#include <queue>

class EnemyPool {
public:
    EnemyPool(size_t initialCapacity = 100);
    ~EnemyPool();
    
    // 从对象池获取一个敌人
    Enemy* Acquire(const glm::vec3& position);
    
    // 将敌人归还给对象池
    void Release(Enemy* enemy);
    
    // 更新所有活跃敌人
    void UpdateAll(float deltaTime, const glm::vec3& playerPos, const std::vector<glm::vec3>& terrainBlocks);
    
    // 获取所有活跃敌人 (用于碰撞检测和渲染)
    const std::vector<Enemy*>& GetActiveEnemies() const;
    
    // 扩展池容量
    void ExpandCapacity(size_t additionalCount);
    
    // 统计信息
    size_t GetActiveCount() const { return m_activeEnemies.size(); }

private:
    std::vector<Enemy*> m_allEnemies;           // 所有分配的内存
    std::queue<Enemy*> m_inactivePool;          // 可用的对象
    std::vector<Enemy*> m_activeEnemies;        // 当前活跃的对象
    
    void RecycleDeadEnemies();
};
