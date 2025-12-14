#include "EnemyPool.h"
#include <algorithm>

EnemyPool::EnemyPool(size_t initialCapacity) {
    ExpandCapacity(initialCapacity);
}

EnemyPool::~EnemyPool() {
    for (auto enemy : m_allEnemies) {
        delete enemy;
    }
    m_allEnemies.clear();
}

Enemy* EnemyPool::Acquire(const glm::vec3& position) {
    if (m_inactivePool.empty()) {
        ExpandCapacity(50);  // 自动扩容
    }
    
    Enemy* enemy = m_inactivePool.front();
    m_inactivePool.pop();
    
    enemy->Activate(position);
    m_activeEnemies.push_back(enemy);
    
    return enemy;
}

void EnemyPool::Release(Enemy* enemy) {
    if (!enemy) return;
    
    // 从活跃列表中移除
    auto it = std::find(m_activeEnemies.begin(), m_activeEnemies.end(), enemy);
    if (it != m_activeEnemies.end()) {
        m_activeEnemies.erase(it);
    }
    
    enemy->DeactivateForPool();
    m_inactivePool.push(enemy);
}

void EnemyPool::UpdateAll(float deltaTime, const glm::vec3& playerPos, const std::vector<glm::vec3>& terrainBlocks) {
    // 1. 更新所有活跃敌人
    for (auto enemy : m_activeEnemies) {
        enemy->Update(deltaTime, playerPos, m_activeEnemies, terrainBlocks);
    }
    
    // 2. 回收完全死亡（尸体消失）的敌人
    RecycleDeadEnemies();
}

const std::vector<Enemy*>& EnemyPool::GetActiveEnemies() const {
    return m_activeEnemies;
}

void EnemyPool::ExpandCapacity(size_t additionalCount) {
    for (size_t i = 0; i < additionalCount; ++i) {
        Enemy* enemy = new Enemy();
        m_allEnemies.push_back(enemy);
        m_inactivePool.push(enemy);
    }
}

void EnemyPool::RecycleDeadEnemies() {
    auto it = m_activeEnemies.begin();
    while (it != m_activeEnemies.end()) {
        if ((*it)->CanBeRecycled()) {
            // 注意：这里我们不能直接调用 Release，因为这会导致迭代器失效问题
            // 所以我们手动做 Release 的逻辑
            Enemy* enemy = *it;
            enemy->DeactivateForPool();
            m_inactivePool.push(enemy);
            
            // 使用 erase 返回下一个迭代器
            it = m_activeEnemies.erase(it);
        } else {
            ++it;
        }
    }
}
