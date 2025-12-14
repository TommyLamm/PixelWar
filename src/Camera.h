// ============================================================================
// Camera.h - 第一人称摄像机类头文件
// 功能: 实现 FPS 风格的摄像机，支持键盘移动和鼠标视角旋转
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * @class Camera
 * @brief 第一人称摄像机类
 * 
 * 实现 FPS 风格的摄像机，支持键盘移动和鼠标视角旋转。
 * 使用欧拉角（Pitch 和 Yaw）来控制摄像机方向，并自动更新摄像机向量。
 */
class Camera
{
public:
    // 摄像机移动方向枚举
    enum class Movement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    // ==================== 构造函数 ====================
    /**
     * @brief 构造函数
     * @param position 初始摄像机位置
     * @param up 世界坐标系上向量（默认为 Y 轴正方向）
     * @param yaw 初始偏航角（默认 -90 度）
     * @param pitch 初始俯仰角（默认 0 度）
     */
    Camera(
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = 0.0f
    );

    // ==================== 获取矩阵接口 ====================
    /**
     * @brief 获取观察矩阵
     * @return 计算得到的观察矩阵
     */
    [[nodiscard]] glm::mat4 GetViewMatrix() const;

    /**
     * @brief 获取投影矩阵
     * @param fov 视场角（Field of View）
     * @param aspectRatio 宽高比
     * @param nearPlane 近裁剪面距离
     * @param farPlane 远裁剪面距离
     * @return 计算得到的投影矩阵
     */
    [[nodiscard]] glm::mat4 GetProjectionMatrix(
        float fov = 45.0f,
        float aspectRatio = 16.0f / 9.0f,
        float nearPlane = 0.1f,
        float farPlane = 100.0f
    ) const;

    // ==================== 输入处理接口 ====================
    /**
     * @brief 处理键盘输入
     * @param direction 移动方向（FORWARD, BACKWARD, LEFT, RIGHT）
     * @param deltaTime 帧间时间差（秒）
     */
    void ProcessKeyboard(Movement direction, float deltaTime);

    /**
     * @brief 处理鼠标移动
     * @param xOffset 鼠标 X 轴移动距离
     * @param yOffset 鼠标 Y 轴移动距离
     * @param constrainPitch 是否限制 Pitch 角度（防止翻转，默认为 true）
     */
    void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

    /**
     * @brief 处理鼠标滚轮输入（FOV 缩放）
     * @param yOffset 滚轮Y轴偏移
     */
    void ProcessMouseScroll(float yOffset);

    // ==================== 物理控制接口 (新增) ====================
    /**
     * @brief 处理跳跃
     */
    void ProcessJump();

    /**
     * @brief 更新物理状态（重力、碰撞）
     * @param deltaTime 帧间时间差
     * @param terrainBlocks 地形方块位置列表
     */
    void UpdatePhysics(float deltaTime, const std::vector<glm::vec3>& terrainBlocks);

    // ==================== Getter 接口 ====================
    /**
     * @brief 获取摄像机位置
     */
    [[nodiscard]] const glm::vec3& GetPosition() const { return m_position; }

    /**
     * @brief 获取摄像机前向向量
     */
    [[nodiscard]] const glm::vec3& GetFront() const { return m_front; }

    /**
     * @brief 获取摄像机右向量
     */
    [[nodiscard]] const glm::vec3& GetRight() const { return m_right; }

    /**
     * @brief 获取摄像机上向量
     */
    [[nodiscard]] const glm::vec3& GetUp() const { return m_up; }

    /**
     * @brief 获取摄像机 Yaw 角度
     */
    [[nodiscard]] float GetYaw() const { return m_yaw; }

    /**
     * @brief 获取摄像机 Pitch 角度
     */
    [[nodiscard]] float GetPitch() const { return m_pitch; }

    [[nodiscard]] float GetFOV() const { return m_fov; }
    [[nodiscard]] float GetMouseSensitivity() const { return m_mouseSensitivity; }

    // ==================== Setter 接口 ====================
    /**
     * @brief 设置摄像机位置
     */
    void SetPosition(const glm::vec3& position)
    {
        m_position = position;
    }

    /**
     * @brief 设置移动速度
     */
    void SetMovementSpeed(float speed)
    {
        m_movementSpeed = glm::max(speed, 0.1f);
    }

    /**
     * @brief 设置鼠标灵敏度
     */
    void SetMouseSensitivity(float sensitivity)
    {
        m_mouseSensitivity = glm::max(sensitivity, 0.01f);
    }

    /**
     * @brief 设置 FOV（视场角）
     */
    void SetFOV(float fov)
    {
        m_fov = glm::clamp(fov, 10.0f, 120.0f);
    }

private:
    // ==================== 摄像机向量属性 ====================
    glm::vec3 m_position;          // 摄像机世界空间位置
    glm::vec3 m_front;             // 摄像机前向向量
    glm::vec3 m_up;                // 摄像机上向量（局部）
    glm::vec3 m_right;             // 摄像机右向量
    glm::vec3 m_worldUp;           // 世界空间上向量（不变）

    // ==================== 欧拉角属性 ====================
    float m_yaw;                   // 偏航角（水平旋转）
    float m_pitch;                 // 俯仰角（垂直旋转）

    // ==================== 移动和视角属性 ====================
    float m_movementSpeed;         // 移动速度（单位/秒）
    float m_mouseSensitivity;      // 鼠标灵敏度
    float m_fov;                   // 视场角（度数）

    // ==================== 物理属性 (新增) ====================
    glm::vec3 m_velocity;          // 当前速度 (主要用于重力)
    float m_gravity;               // 重力加速度
    float m_jumpForce;             // 跳跃力度
    bool m_isGrounded;             // 是否在地面上
    float m_playerHeight;          // 玩家高度 (眼睛到脚底)
    float m_playerRadius;          // 玩家碰撞半径

    // ==================== 常量 ====================
    static constexpr float MAX_PITCH = 89.0f;      // Pitch 最大值
    static constexpr float MIN_PITCH = -89.0f;     // Pitch 最小值
    static constexpr float DEFAULT_SPEED = 2.5f;   // 默认移动速度
    static constexpr float DEFAULT_SENSITIVITY = 0.1f; // 默认鼠标灵敏度

    // ==================== 私有方法 ====================
    /**
     * @brief 根据欧拉角更新摄像机向量
     * 
     * 这个函数应该在 Pitch 或 Yaw 改变后调用，
     * 用于重新计算前向、右、上向量。
     */
    void UpdateCameraVectors();
};

