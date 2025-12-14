#include "AIDirector.h"
#include <glm/glm.hpp>
#include <random>
#include <iostream>

AIDirector::AIDirector(EnemyPool* enemyPool)
    : m_enemyPool(enemyPool),
      m_directorState(DirectorState::Calm),
      m_stateTimer(0.0f),
      m_spawnTimer(0.0f),
      m_hordeActive(false),
      m_hordeEnemiesSpawned(0),
      m_hordeTarget(20),
      m_hordeDuration(0.0f),
      m_tension(0.0f)
{
}

void AIDirector::Update(float deltaTime, bool playerIsShooting) {
    UpdateTension(playerIsShooting);
    
    m_stateTimer += deltaTime;
    m_spawnTimer += deltaTime;
    
    size_t currentCount = m_enemyPool->GetActiveCount();
    
    switch (m_directorState) {
        case DirectorState::Calm: {
            // 如果压力过大，进入构建期
            if (m_tension > 3.0f) {
                m_directorState = DirectorState::Building;
                m_stateTimer = 0.0f;
                std::cout << "[AI导演] 进入紧张期..." << std::endl;
            }
            
            // 偶尔生成零星敌人 (每 3 秒 1 个)
            if (m_spawnTimer > 3.0f && currentCount < 5) {
                SpawnWave(1);
                m_spawnTimer = 0.0f;
            }
            break;
        }
        case DirectorState::Building: {
            // 压力继续积累或持续一定时间后触发尸潮
            if (m_tension > 8.0f || m_stateTimer > 5.0f) {
                TriggerHorde(20);
                m_directorState = DirectorState::Horde;
                std::cout << "[AI导演] ⚠️ 尸潮来袭! ⚠️" << std::endl;
            } else if (m_spawnTimer > 1.5f && currentCount < 10) {
                SpawnWave(2);
                m_spawnTimer = 0.0f;
            }
            break;
        }
        case DirectorState::Horde: {
            m_hordeDuration += deltaTime;
            
            // 快速生成敌人
            if (m_spawnTimer > 0.2f && m_hordeEnemiesSpawned < m_hordeTarget) {
                SpawnWave(1);
                m_hordeEnemiesSpawned++;
                m_spawnTimer = 0.0f;
            }
            
            // 尸潮结束条件
            if (m_hordeDuration > 15.0f || (m_hordeEnemiesSpawned >= m_hordeTarget && currentCount < 3)) {
                m_hordeActive = false;
                m_directorState = DirectorState::Calm;
                m_tension = 0.0f; // 重置压力
                m_stateTimer = 0.0f;
                std::cout << "[AI导演] 尸潮结束，进入平静期。" << std::endl;
            }
            break;
        }
    }
}

void AIDirector::UpdateTension(bool playerIsShooting) {
    // 玩家射击会增加压力值
    if (playerIsShooting) {
        m_tension += 0.05f;
    }
    
    // 压力值随时间自然衰减
    m_tension = glm::max(0.0f, m_tension - 0.01f);
}

void AIDirector::TriggerHorde(int enemyCount) {
    m_hordeTarget = enemyCount;
    m_hordeEnemiesSpawned = 0;
    m_hordeActive = true;
    m_hordeDuration = 0.0f;
}

void AIDirector::SpawnWave(int count) {
    for (int i = 0; i < count; ++i) {
        glm::vec3 spawnPos = GetRandomSpawnPosition();
        m_enemyPool->Acquire(spawnPos);
    }
}

glm::vec3 AIDirector::GetRandomSpawnPosition() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> distSide(0, 3);
    std::uniform_real_distribution<float> distCoord(-20.0f, 20.0f); // 稍微扩大生成范围
    
    int side = distSide(rng);
    glm::vec3 pos(0.0f); // 初始化，防止警告

    // 在 20x20 区域边缘生成
    switch (side)
    {
    case 0: pos = glm::vec3(distCoord(rng), 0.9f, -20.0f); break;
    case 1: pos = glm::vec3(distCoord(rng), 0.9f, 20.0f); break;
    case 2: pos = glm::vec3(-20.0f, 0.9f, distCoord(rng)); break;
    case 3: pos = glm::vec3(20.0f, 0.9f, distCoord(rng)); break;
    }
    
    return pos;
}
