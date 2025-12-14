#define GLM_ENABLE_EXPERIMENTAL
#include "Enemy.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

Enemy::Enemy()
    : m_state(EnemyState::Inactive),
      m_position(0.0f),
      m_velocity(0.0f),
      m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      m_color(0.8f, 0.1f, 0.1f),
      m_scale(0.8f, 1.8f, 0.8f),
      m_speed(2.5f),
      m_health(100.0f),
      m_deathTimer(0.0f),
      m_deathDuration(2.0f)
{
}

void Enemy::Activate(const glm::vec3& position)
{
    m_state = EnemyState::Active;
    m_position = position;
    m_health = 100.0f;
    m_deathTimer = 0.0f;
    m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_color = glm::vec3(0.8f, 0.1f, 0.1f); // 重置颜色
}

// 简单的 AABB 碰撞检测结构体 (需要在 Enemy.cpp 也能访问，最好提出来放到 Physics.h，这里先复制一份)
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};
// 检查两个 AABB 是否重叠
static bool CheckCollision(const AABB& one, const AABB& two) {
    bool collisionX = one.max.x >= two.min.x && two.max.x >= one.min.x;
    bool collisionY = one.max.y >= two.min.y && two.max.y >= one.min.y;
    bool collisionZ = one.max.z >= two.min.z && two.max.z >= one.min.z;
    return collisionX && collisionY && collisionZ;
}

void Enemy::Update(float deltaTime, const glm::vec3& playerPos, const std::vector<Enemy*>& activeEnemies, const std::vector<glm::vec3>& terrainBlocks)
{
    if (m_state == EnemyState::Inactive) return;

    // 先应用重力 (假设所有非 Inactive 状态都受重力影响)
    float gravity = 25.0f;
    m_velocity.y -= gravity * deltaTime;
    if (m_velocity.y < -20.0f) m_velocity.y = -20.0f;

    // 预测下一帧位置 (在碰撞检测前先移动)
    glm::vec3 nextPos = m_position;
    
    if (m_state == EnemyState::Active)
    {
        // 1. 追踪 (Seek)
        glm::vec3 target = playerPos;
        target.y = m_position.y; 
        glm::vec3 direction = target - m_position;
        
        // 面朝玩家
        if (glm::length(direction) > 0.1f) {
            glm::vec3 dirNorm = glm::normalize(direction);
            float angle = std::atan2(dirNorm.x, dirNorm.z);
            m_rotation = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::vec3 seekForce(0.0f);
        if (glm::length(direction) > 0.1f) {
            seekForce = glm::normalize(direction);
        }

        // 2. 分离 (Separation)
        glm::vec3 separationForce = CalculateSeparation(activeEnemies);

        // 3. 移动力
        glm::vec3 moveDir = seekForce * 1.0f + separationForce * 1.5f;
        if (glm::length(moveDir) > 0.1f) {
            moveDir = glm::normalize(moveDir);
        }
        
        // 在 XZ 平面移动
        nextPos.x += moveDir.x * m_speed * deltaTime;
        nextPos.z += moveDir.z * m_speed * deltaTime;
    }
    
    // Y 轴移动 (重力)
    nextPos.y += m_velocity.y * deltaTime;

    // ------------------- 地形碰撞检测 (与 Camera 类似逻辑) -------------------
    
    glm::vec3 halfSize = m_scale * 0.5f; // 假设 m_scale 是全尺寸，halfSize 是半尺寸
    AABB enemyBox;
    // 寻找最近的方块
    std::vector<AABB> nearbyBlocks;
    for (const auto& blockPos : terrainBlocks) {
        if (glm::abs(blockPos.x - nextPos.x) > 1.5f || 
            glm::abs(blockPos.z - nextPos.z) > 1.5f ||
            glm::abs(blockPos.y - nextPos.y) > 2.5f) continue;
            
        AABB blockBox;
        blockBox.min = blockPos - glm::vec3(0.5f);
        blockBox.max = blockPos + glm::vec3(0.5f);
        nearbyBlocks.push_back(blockBox);
    }

    // 迭代解决碰撞
    int iterations = 4;
    bool isGrounded = false;
    
    while (iterations--) {
        bool collided = false;
        enemyBox.min = nextPos - halfSize;
        enemyBox.max = nextPos + halfSize;

        for (const auto& blockBox : nearbyBlocks) {
            if (CheckCollision(enemyBox, blockBox)) {
                float overlapX = std::min(enemyBox.max.x, blockBox.max.x) - std::max(enemyBox.min.x, blockBox.min.x);
                float overlapY = std::min(enemyBox.max.y, blockBox.max.y) - std::max(enemyBox.min.y, blockBox.min.y);
                float overlapZ = std::min(enemyBox.max.z, blockBox.max.z) - std::max(enemyBox.min.z, blockBox.min.z);

                if (overlapX < overlapY && overlapX < overlapZ) {
                    if (nextPos.x > blockBox.min.x + 0.5f) nextPos.x += overlapX;
                    else nextPos.x -= overlapX;
                } else if (overlapZ < overlapY && overlapZ < overlapX) {
                    if (nextPos.z > blockBox.min.z + 0.5f) nextPos.z += overlapZ;
                    else nextPos.z -= overlapZ;
                } else {
                    if (nextPos.y > blockBox.min.y + 0.5f) {
                        nextPos.y += overlapY;
                        m_velocity.y = 0.0f;
                        isGrounded = true;
                    } else {
                        nextPos.y -= overlapY;
                        if (m_velocity.y > 0) m_velocity.y = 0;
                    }
                }
                collided = true;
            }
        }
        if (!collided) break;
    }
    
    // 防止掉出地图
    if (nextPos.y < -20.0f) {
        nextPos.y = 20.0f;
        m_velocity.y = 0.0f;
    }

    m_position = nextPos;

    // ------------------- 状态更新 (死亡动画等) -------------------

    if (m_state == EnemyState::Dying)
    {
        m_deathTimer += deltaTime;
        float animDuration = 0.5f;
        if (m_deathTimer < animDuration) {
             m_rotation = glm::rotate(m_rotation, glm::radians(90.0f * deltaTime / animDuration), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else {
            m_state = EnemyState::Dead;
            m_deathTimer = 0.0f; 
        }
    }
    else if (m_state == EnemyState::Dead)
    {
        m_deathTimer += deltaTime;
        m_color = glm::vec3(0.2f, 0.0f, 0.0f);
    }
}

bool Enemy::TakeDamage(float damage)
{
    if (m_state != EnemyState::Active) return false;
    
    m_health -= damage;
    if (m_health <= 0) {
        Kill();
        return true;
    }
    return false;
}

void Enemy::Kill()
{
    if (m_state == EnemyState::Active) {
        m_state = EnemyState::Dying;
        m_deathTimer = 0.0f;
    }
}

bool Enemy::CanBeRecycled() const
{
    return m_state == EnemyState::Dead && m_deathTimer > m_deathDuration;
}

glm::vec3 Enemy::CalculateSeparation(const std::vector<Enemy*>& activeEnemies)
{
    glm::vec3 separation(0.0f);
    int neighbors = 0;
    float separationRadius = 1.5f;

    for (const auto& other : activeEnemies) {
        if (other == this || !other->IsActive()) continue;

        float dist = glm::distance(m_position, other->m_position);
        if (dist > 0.001f && dist < separationRadius) {
            glm::vec3 push = m_position - other->m_position;
            push = glm::normalize(push) / dist; 
            separation += push;
            neighbors++;
        }
    }

    if (neighbors > 0) separation /= static_cast<float>(neighbors);
    separation.y = 0.0f;
    return separation;
}

void Enemy::getAABB(glm::vec3& min, glm::vec3& max) const
{
    glm::vec3 halfSize = m_scale * 0.5f;
    min = m_position - halfSize;
    max = m_position + halfSize;
}
