#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

enum class EnemyState {
    Inactive,    // 在对象池中
    Active,      // 存活
    Dying,       // 死亡动画播放中
    Dead         // 尸体残留
};

class Enemy {
public:
    Enemy();
    
    // 初始化敌人
    void Activate(const glm::vec3& position);
    
    // 更新逻辑
    void Update(float deltaTime, const glm::vec3& playerPos, const std::vector<Enemy*>& activeEnemies, const std::vector<glm::vec3>& terrainBlocks);
    
    // 受到伤害，返回是否被击杀
    bool TakeDamage(float damage);
    
    // 立即杀死 (用于测试或清理)
    void Kill();
    
    // Getters
    EnemyState GetState() const { return m_state; }
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetColor() const { return m_color; }
    glm::vec3 GetScale() const { return m_scale; }
    glm::quat GetRotation() const { return m_rotation; }
    bool IsActive() const { return m_state == EnemyState::Active; }
    bool CanBeRecycled() const;
    float GetHealth() const { return m_health; }

    // AABB 获取
    void getAABB(glm::vec3& min, glm::vec3& max) const;

private:
    EnemyState m_state;
    
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::quat m_rotation; // 使用四元数处理旋转 (特别是倒地动画)
    
    // 渲染属性
    glm::vec3 m_color;
    glm::vec3 m_scale;
    
    // 游戏属性
    float m_speed;
    float m_health;
    
    // 死亡处理
    float m_deathTimer;
    float m_deathDuration; // 尸体残留时间

    // 内部逻辑
    glm::vec3 CalculateSeparation(const std::vector<Enemy*>& activeEnemies);
};
