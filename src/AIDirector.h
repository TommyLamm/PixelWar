#pragma once

#include "EnemyPool.h"

class AIDirector {
public:
    AIDirector(EnemyPool* enemyPool);
    
    // 更新导演逻辑
    void Update(float deltaTime, bool playerIsShooting);
    
    // 查询状态
    bool IsHordeActive() const { return m_hordeActive; }

private:
    enum class DirectorState {
        Calm,      // 平静期
        Building,  // 紧张积累期
        Horde      // 尸潮爆发
    };
    
    EnemyPool* m_enemyPool;
    DirectorState m_directorState;
    
    // 计时器
    float m_stateTimer;
    float m_spawnTimer;
    
    // 尸潮参数
    bool m_hordeActive;
    int m_hordeEnemiesSpawned;
    int m_hordeTarget;
    float m_hordeDuration;
    
    // 压力值系统
    float m_tension;
    
    // 辅助函数
    void TriggerHorde(int enemyCount);
    void SpawnWave(int count);
    void UpdateTension(bool playerIsShooting);
    glm::vec3 GetRandomSpawnPosition();
};
