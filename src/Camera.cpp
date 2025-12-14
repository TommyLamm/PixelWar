// ============================================================================
// Camera.cpp - 第一人称摄像机类实现文件
// 功能: 实现 FPS 风格的摄像机，支持键盘移动和鼠标视角旋转
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#include "Camera.h"
#include <algorithm>

// ==================== 构造函数实现 ====================
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_position(position),
      m_worldUp(up),
      m_yaw(yaw),
      m_pitch(pitch),
      m_movementSpeed(DEFAULT_SPEED),
      m_mouseSensitivity(DEFAULT_SENSITIVITY),
      m_fov(45.0f),
      m_velocity(0.0f),
      m_gravity(25.0f), // 适中的重力
      m_jumpForce(10.0f),
      m_isGrounded(false),
      m_playerHeight(1.8f),
      m_playerRadius(0.3f)
{
    // 初始化摄像机向量
    UpdateCameraVectors();
}

// ==================== 公开接口实现 ====================

glm::mat4 Camera::GetViewMatrix() const
{
    // 使用 GLM 的 lookAt 函数计算观察矩阵
    // 参数：摄像机位置 + 前向向量（看向的点） + 上向量
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::GetProjectionMatrix(
    float fov,
    float aspectRatio,
    float nearPlane,
    float farPlane) const
{
    // 使用 GLM 的 perspective 函数计算投影矩阵
    // 参数：FOV（度数转弧度）、宽高比、近裁剪面、远裁剪面
    return glm::perspective(
        glm::radians(m_fov),
        aspectRatio,
        nearPlane,
        farPlane
    );
}

void Camera::ProcessKeyboard(Movement direction, float deltaTime)
{
    // 计算本帧移动的距离
    float velocity = m_movementSpeed * deltaTime;

    // 创建只包含 XZ 平面分量的前向向量 (用于地面移动)
    glm::vec3 frontXZ = glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z));
    // 右向量本身通常已经是水平的（如果世界 Up 是 Y 轴），但为了保险也可以做一次投影
    glm::vec3 rightXZ = glm::normalize(glm::vec3(m_right.x, 0.0f, m_right.z));

    glm::vec3 moveDir(0.0f);

    switch (direction)
    {
        case Movement::FORWARD:
            moveDir += frontXZ;
            break;
        case Movement::BACKWARD:
            moveDir -= frontXZ;
            break;
        case Movement::LEFT:
            moveDir -= rightXZ;
            break;
        case Movement::RIGHT:
            moveDir += rightXZ;
            break;
    }
    
    // 更新水平位置 (垂直位置由物理引擎控制)
    if (glm::length(moveDir) > 0.0f)
        m_position += glm::normalize(moveDir) * velocity;
}

void Camera::ProcessJump()
{
    if (m_isGrounded)
    {
        m_velocity.y = m_jumpForce;
        m_isGrounded = false;
    }
}

// 简单的 AABB 碰撞检测结构体
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

void Camera::UpdatePhysics(float deltaTime, const std::vector<glm::vec3>& terrainBlocks)
{
    // 1. 应用重力
    m_velocity.y -= m_gravity * deltaTime;
    
    // 限制最大下落速度
    if (m_velocity.y < -20.0f) m_velocity.y = -20.0f;

    // 2. 预测垂直移动后的位置
    glm::vec3 nextPos = m_position;
    nextPos.y += m_velocity.y * deltaTime;

    // 玩家包围盒 (假设玩家高 1.8，宽 0.6)
    // 包围盒底部是脚底，顶部是头顶
    // m_position 通常是摄像机(眼睛)位置，假设眼睛在 1.7 高度
    // 所以脚底是 m_position.y - 1.7
    float eyeHeight = 1.7f;
    float footY = nextPos.y - eyeHeight;
    
    // AABB playerBox; <-- 删除了这里的重复定义
    // playerBox.min = glm::vec3(nextPos.x - m_playerRadius, footY, nextPos.z - m_playerRadius);
    // playerBox.max = glm::vec3(nextPos.x + m_playerRadius, footY + m_playerHeight, nextPos.z + m_playerRadius);

    m_isGrounded = false;

    // 3. 地形碰撞检测
    // 采用更完整的 3D AABB 碰撞解决，支持“滑墙”
    
    // 玩家 AABB 参数
    float halfW = m_playerRadius;
    float halfH = m_playerHeight / 2.0f;
    glm::vec3 playerSize(halfW, halfH, halfW);

    // 假设玩家中心在 (x, y + halfH, z) 
    // m_position 是眼睛位置 (y=1.7)，脚底是 m_position.y - 1.7
    // 所以中心高度是 (m_position.y - 1.7) + 0.9 = m_position.y - 0.8
    glm::vec3 playerCenter = nextPos;
    playerCenter.y -= 0.8f; 

    AABB playerBox; // 这里的定义是需要的
    playerBox.min = playerCenter - playerSize;
    playerBox.max = playerCenter + playerSize;

    // 寻找最近的方块

    std::vector<AABB> nearbyBlocks;
    for (const auto& blockPos : terrainBlocks) {
        // 快速剔除
        if (glm::abs(blockPos.x - playerCenter.x) > 1.5f || 
            glm::abs(blockPos.z - playerCenter.z) > 1.5f ||
            glm::abs(blockPos.y - playerCenter.y) > 2.5f) continue;
            
        AABB blockBox;
        blockBox.min = blockPos - glm::vec3(0.5f);
        blockBox.max = blockPos + glm::vec3(0.5f);
        nearbyBlocks.push_back(blockBox);
    }

    // 迭代解决碰撞 (处理多个方块的共同作用)
    int iterations = 4;
    while (iterations--) {
        bool collided = false;
        // 更新包围盒
        playerBox.min = playerCenter - playerSize;
        playerBox.max = playerCenter + playerSize;

        for (const auto& blockBox : nearbyBlocks) {
            if (CheckCollision(playerBox, blockBox)) {
                // 计算重叠量
                float overlapX = std::min(playerBox.max.x, blockBox.max.x) - std::max(playerBox.min.x, blockBox.min.x);
                float overlapY = std::min(playerBox.max.y, blockBox.max.y) - std::max(playerBox.min.y, blockBox.min.y);
                float overlapZ = std::min(playerBox.max.z, blockBox.max.z) - std::max(playerBox.min.z, blockBox.min.z);

                // 找到最小穿透轴进行分离
                if (overlapX < overlapY && overlapX < overlapZ) {
                    // X 轴碰撞 (墙壁)
                    if (playerCenter.x > blockBox.min.x + 0.5f) // 在右侧
                        playerCenter.x += overlapX;
                    else
                        playerCenter.x -= overlapX;
                    // 不清空 m_velocity.y，允许贴墙掉落
                } else if (overlapZ < overlapY && overlapZ < overlapX) {
                    // Z 轴碰撞 (墙壁)
                    if (playerCenter.z > blockBox.min.z + 0.5f) // 在后方
                        playerCenter.z += overlapZ;
                    else
                        playerCenter.z -= overlapZ;
                } else {
                    // Y 轴碰撞 (地面或天花板)
                    if (playerCenter.y > blockBox.min.y + 0.5f) { // 在上方 (落地)
                        playerCenter.y += overlapY;
                        m_velocity.y = 0.0f;
                        m_isGrounded = true;
                    } else { // 在下方 (顶头)
                        playerCenter.y -= overlapY;
                        if (m_velocity.y > 0) m_velocity.y = 0;
                    }
                }
                collided = true;
            }
        }
        if (!collided) break;
    }
    
    // 更新回 m_position (眼睛位置)
    m_position = playerCenter;
    m_position.y += 0.8f;

    
    // 防止掉出地图太远
    if (m_position.y < -20.0f) {
        m_position.y = 20.0f;
        m_velocity.y = 0.0f;
    }
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    // 应用鼠标灵敏度缩放偏移
    xOffset *= m_mouseSensitivity;
    yOffset *= m_mouseSensitivity;

    // 更新欧拉角
    // Yaw：水平旋转（绕世界 Y 轴）
    m_yaw -= xOffset; // 修正：减去 xOffset 以获得正确的鼠标左/右移动方向

    // Pitch：垂直旋转（绕局部 X 轴）
    m_pitch += yOffset;

    // 可选：限制 Pitch 角度，防止摄像机翻转
    if (constrainPitch)
    {
        m_pitch = glm::clamp(m_pitch, MIN_PITCH, MAX_PITCH);
    }

    // 根据新的欧拉角更新摄像机向量
    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset)
{
    // 根据滚轮方向调整 FOV
    m_fov -= yOffset;
    // 限制 FOV 在合理范围内
    m_fov = glm::clamp(m_fov, 1.0f, 45.0f);
}

// ==================== 私有方法实现 ====================

void Camera::UpdateCameraVectors()
{
    // 将欧拉角转换为弧度
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    // 计算新的前向向量
    // 使用球面坐标公式：
    // x = cos(pitch) * sin(yaw)
    // y = sin(pitch)
    // z = cos(pitch) * cos(yaw)
    glm::vec3 front;
    front.x = glm::cos(pitchRad) * glm::sin(yawRad);
    front.y = glm::sin(pitchRad);
    front.z = glm::cos(pitchRad) * glm::cos(yawRad);

    // 归一化前向向量
    m_front = glm::normalize(front);

    // 计算右向量：前向向量 × 世界上向量
    // 右向量垂直于前向向量和世界上向量所定义的平面
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));

    // 重新计算上向量：右向量 × 前向向量
    // 这确保了前向、右、上向量形成一个正交基
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

