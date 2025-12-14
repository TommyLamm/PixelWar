// ============================================================================
// OpenGL 基础渲染框架 - 主程序文件
// 功能: 初始化 GLFW/GLAD 窗口系统，实现游戏循环和渲染管道
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#include <iostream>
#include <string>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

// GLAD OpenGL 函数加载器
// 注意：GLAD 必须放在 GLFW 之前，否则 GLFW 会自动包含 OpenGL 头文件，
// 导致 GLAD 检测到 __gl_h_ 已定义而报错
#include <glad/glad.h>

// GLFW 窗口和输入管理库
// GLFW 会检测到 GLAD 已包含 OpenGL 头文件，自动跳过其自身的包含
#include <GLFW/glfw3.h>

// GLM 数学库
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 核心组件
#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "InstancedMesh.h" // 新增
#include "Geometry.h"
#include "Enemy.h"
#include "EnemyPool.h"
#include "AIDirector.h"
#include "Settings.h"
#include <vector>
#include <random>
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <sstream>

// ------------------------- Perlin Noise 2D ----------------------------------
struct Perlin2D {
    std::uint8_t p[512];

    explicit Perlin2D(std::uint32_t seed = 1337u) {
        // Simple LCG to shuffle permutation deterministically by seed.
        std::uint8_t perm[256];
        for (int i = 0; i < 256; ++i) perm[i] = static_cast<std::uint8_t>(i);

        auto lcg = [s = seed]() mutable -> std::uint32_t {
            s = s * 1664525u + 1013904223u;
            return s;
        };

        for (int i = 255; i > 0; --i) {
            int j = static_cast<int>(lcg() % (i + 1));
            std::uint8_t tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }
        for (int i = 0; i < 256; ++i) {
            p[i] = perm[i];
            p[i + 256] = perm[i];
        }
    }

    static float fade(float t) {
        return t * t * t * (t * (t * 6.f - 15.f) + 10.f); // 6t^5 - 15t^4 + 10t^3
    }

    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    static float grad2(int hash, float x, float y) {
        // 8 gradient directions
        switch (hash & 7) {
            case 0: return  x + y;
            case 1: return -x + y;
            case 2: return  x - y;
            case 3: return -x - y;
            case 4: return  x;
            case 5: return -x;
            case 6: return  y;
            default: return -y;
        }
    }

    // 2D Perlin noise in [-1, 1]
    float noise(float x, float y) const {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        float xf = x - std::floor(x);
        float yf = y - std::floor(y);

        float u = fade(xf);
        float v = fade(yf);

        int aa = p[p[X] + Y];
        int ab = p[p[X] + Y + 1];
        int ba = p[p[X + 1] + Y];
        int bb = p[p[X + 1] + Y + 1];

        float x1 = lerp(grad2(aa, xf,     yf    ),
                        grad2(ba, xf-1.f, yf    ), u);
        float x2 = lerp(grad2(ab, xf,     yf-1.f),
                        grad2(bb, xf-1.f, yf-1.f), u);

        return lerp(x1, x2, v); // [-1,1] ish
    }

    // Fractal Brownian Motion (FBM) using multiple octaves of 2D noise.
    float fbm(float x, float y, int octaves = 5,
              float lacunarity = 2.0f, float gain = 0.5f) const
    {
        float sum = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        float maxSum = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            sum += noise(x * freq, y * freq) * amp;
            maxSum += amp;
            freq *= lacunarity;
            amp  *= gain;
        }
        // Normalize to [0,1]
        return 0.5f * (sum / maxSum + 1.0f);
    }
};

enum class BlockType : std::uint8_t {
    Air = 0,
    Water,
    Sand,
    Grass,
    Dirt,
    Stone,
    Snow,
    Wood,
    Leaves
};

struct TerrainParams {
    float baseAmplitude = 30.0f;     
    float baseFrequency = 0.02f;   
    int   baseOctaves   = 4;

    float waterLevel = 5.0f;
    float beachHeight = 2.0f;        
    float snowHeight  = 22.0f;      

    float treeThreshold = 0.985f; // 只有非常高的随机值才生成树
};

inline float terrainHeight(const Perlin2D& perlin, const TerrainParams& tp, int x, int z)
{
    float nx = x * tp.baseFrequency;
    float nz = z * tp.baseFrequency;
    float h01 = perlin.fbm(nx, nz, tp.baseOctaves); 
    return h01 * tp.baseAmplitude;
}

inline glm::vec3 getBlockColor(BlockType t)
{
    switch (t) {
        case BlockType::Water:  return glm::vec3(0.0f, 0.4f, 0.8f);
        case BlockType::Sand:   return glm::vec3(0.9f, 0.85f, 0.6f);
        case BlockType::Grass:  return glm::vec3(0.2f, 0.6f, 0.2f);
        case BlockType::Dirt:   return glm::vec3(0.4f, 0.25f, 0.15f);
        case BlockType::Stone:  return glm::vec3(0.5f, 0.5f, 0.5f);
        case BlockType::Snow:   return glm::vec3(0.95f, 0.95f, 0.98f);
        case BlockType::Wood:   return glm::vec3(0.4f, 0.2f, 0.1f);
        case BlockType::Leaves: return glm::vec3(0.1f, 0.5f, 0.1f);
        default:                return glm::vec3(1.0f);
    }
}

// ============================================================================
// 配置常量定义
// ============================================================================

// 窗口配置参数
constexpr int WINDOW_WIDTH = 1600;      // 窗口宽度 (像素)
constexpr int WINDOW_HEIGHT = 900;      // 窗口高度 (像素)
constexpr const char* WINDOW_TITLE = "Pixel War";
int g_windowWidth = WINDOW_WIDTH;
int g_windowHeight = WINDOW_HEIGHT;
constexpr float ENEMY_MAX_HEALTH = 100.0f;
constexpr float BULLET_DAMAGE = 20.0f; // about 5 shots to kill (100 HP)

struct ResolutionOption
{
    int width;
    int height;
    const char* label;
};

constexpr ResolutionOption RESOLUTION_OPTIONS[] = {
    {1280, 720,  "1280x720"},
    {1600, 900,  "1600x900"},
    {1920, 1080, "1920x1080"}
};
constexpr int RESOLUTION_COUNT = static_cast<int>(sizeof(RESOLUTION_OPTIONS) / sizeof(ResolutionOption));
int g_resolutionIndex = -1; // -1 means custom (e.g., drag-resize)
std::string g_resolutionLabel = "Custom";

#ifdef _WIN32
std::string g_sfxShootPath = "assets/sfx_shoot.wav";
std::string g_sfxHitPath = "assets/sfx_hit.wav";
std::string g_sfxKillPath = "assets/sfx_kill.wav";
#endif

// 射击相关配置
constexpr float FIRE_RATE = 0.1f;       // 射速 (秒/发)
constexpr float SPREAD_AMOUNT = 0.05f;  // 弹道散布程度
float g_lastShootTime = 0.0f;           // 上次射击时间

struct BulletTrail {
    glm::vec3 start;
    glm::vec3 end;
    float timeAlive;
    float maxLifetime;
    glm::vec4 color;
};

std::vector<BulletTrail> g_bulletTrails;
Shader* g_lineShader = nullptr;
unsigned int g_lineVAO = 0;
unsigned int g_lineVBO = 0;

#include <random>

// 渲染配置参数
constexpr float CLEAR_COLOR_R = 0.2f;   // 清除颜色 - 红分量
constexpr float CLEAR_COLOR_G = 0.2f;   // 清除颜色 - 绿分量
constexpr float CLEAR_COLOR_B = 0.2f;   // 清除颜色 - 蓝分量
constexpr float CLEAR_COLOR_A = 1.0f;   // 清除颜色 - Alpha 分量 (不透明)

// ============================================================================
// 全局变量声明
// ============================================================================

// 全局窗口指针，用于回调函数中访问窗口对象
GLFWwindow* g_window = nullptr;

// 程序运行状态标志
bool g_running = true;
bool g_isPaused = false; // 暂停状态

// 全局摄像机对象
Camera g_camera(glm::vec3(0.0f, 2.0f, 6.0f));

// 全局资源
Shader* g_shader = nullptr;
Shader* g_instancedShader = nullptr; 
Shader* g_crosshairShader = nullptr; // 准星 Shader
Mesh* g_cubeMesh = nullptr; 
InstancedMesh* g_terrainMesh = nullptr;
Mesh* g_planeMesh = nullptr;
unsigned int g_crosshairVAO = 0, g_crosshairVBO = 0; // 准星资源
unsigned int g_uiVAO = 0, g_uiVBO = 0; // UI 资源 (Quad)

// 地形数据缓存 (用于物理碰撞)
std::vector<glm::vec3> g_terrainPositions;


// 全局资源
// ... (其他资源)
// ...

// 游戏对象结构体
struct CubeObject
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 scale;

    // 获取 AABB
    void getAABB(glm::vec3& min, glm::vec3& max) const
    {
        glm::vec3 halfSize = scale * 0.5f;
        min = position - halfSize;
        max = position + halfSize;
    }
};

// 简单的空间网格 (Spatial Grid) 用于加速射线检测
// 地图范围 64 -> -64 到 +63
// x, z 偏移 +64 -> 索引 0 到 127
// y 范围 -10 到 30+ -> 偏移 +10 -> 索引 0 到 64 足够
struct SpatialGrid {
    static const int SIZE_X = 128;
    static const int SIZE_Y = 64;
    static const int SIZE_Z = 128;
    
    // Store CubeObject pointers (or indices).
    // For simplicity we only track whether a block exists; if it does, we look up the CubeObject.
    // Another option is storing a vector of CubeObject*.
    // Fastest would be std::vector<CubeObject*> per cell,
    // but that consumes more memory.
    // Shooting really just needs the hit position; CubeObject is mainly for recoloring.
    // We can store CubeObject*.
    
    std::vector<CubeObject*> grid[SIZE_X][SIZE_Y][SIZE_Z];
    
    void Clear() {
        for(int x=0; x<SIZE_X; ++x)
            for(int y=0; y<SIZE_Y; ++y)
                for(int z=0; z<SIZE_Z; ++z)
                    grid[x][y][z].clear();
    }
    
    void Add(CubeObject* cube) {
        int ix = (int)std::floor(cube->position.x) + 64;
        int iy = (int)std::floor(cube->position.y) + 10;
        int iz = (int)std::floor(cube->position.z) + 64;
        
        if (ix >= 0 && ix < SIZE_X && iy >= 0 && iy < SIZE_Y && iz >= 0 && iz < SIZE_Z) {
            grid[ix][iy][iz].push_back(cube);
        }
    }
    
    std::vector<CubeObject*>* Get(int x, int y, int z) {
        int ix = x + 64;
        int iy = y + 10;
        int iz = z + 64;
        if (ix >= 0 && ix < SIZE_X && iy >= 0 && iy < SIZE_Y && iz >= 0 && iz < SIZE_Z) {
            return &grid[ix][iy][iz];
        }
        return nullptr;
    }
};

SpatialGrid g_spatialGrid;

// 游戏对象结构体 (Removed from here)
// struct CubeObject ... 


std::vector<CubeObject> g_cubes;
// std::vector<Enemy> g_enemies; // 移除旧的 vector
EnemyPool* g_enemyPool = nullptr;
AIDirector* g_director = nullptr;

// 射线结构体
struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;
};

// 鼠标输入相关变量
bool g_firstMouse = true;              // 首次鼠标移动标志
bool g_isShooting = false;             // 是否正在射击
float g_lastX = WINDOW_WIDTH / 2.0f;   // 上次鼠标 X 位置
float g_lastY = WINDOW_HEIGHT / 2.0f;  // 上次鼠标 Y 位置

// 时间相关变量
float g_deltaTime = 0.0f;              // 当前帧与上一帧的时间差
float g_lastFrame = 0.0f;               // 上一帧的时间

// ============================================================================
// 函数原型声明
// ============================================================================

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

/**
 * @brief 初始化 GLFW 库和创建渲染窗口
 * @return 成功返回 true，失败返回 false
 * 
 * 功能详解:
 *  - 初始化 GLFW 库
 *  - 配置 OpenGL 上下文属性 (版本、核心模式等)
 *  - 创建窗口
 *  - 使用 glfwMakeContextCurrent 将 OpenGL 上下文绑定到线程
 */
bool InitializeWindow();

/**
 * @brief 初始化 GLAD 库并加载 OpenGL 函数指针
 * @return 成功返回 true，失败返回 false
 * 
 * 功能详解:
 *  - 使用 GLAD 加载所有 OpenGL 函数指针
 *  - 验证 OpenGL 扩展支持情况
 *  - 输出 OpenGL 版本信息供调试
 */
bool InitializeGLAD();

/**
 * @brief 射线与 AABB 相交检测 (Slab Method)
 * @param ray 射线
 * @param boxMin AABB 最小点
 * @param boxMax AABB 最大点
 * @param t 返回相交距离
 * @return 是否相交
 */
bool intersectRayAABB(const Ray& ray, const glm::vec3& invDir, const glm::vec3& boxMin, const glm::vec3& boxMax, float& t);

/**
 * @brief 处理射击逻辑
 */
void ProcessShooting();

/**
 * @brief 处理鼠标点击事件的回调函数
 */
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

/**
 * @brief 初始化场景资源（着色器、网格、物体位置）
 */
bool InitializeScene();

/**
 * @brief 处理键盘输入事件的回调函数
 * 
 * @param window GLFW 窗口对象
 * @param key 按下的键码 (GLFW_KEY_*)
 * @param scancode 扫描码 (与平台相关)
 * @param action 键盘动作 (GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT)
 * @param mods 修饰键掩码 (Ctrl, Shift, Alt)
 * 
 * 功能详解:
 *  - 检测 ESC 键按下事件，触发程序退出
 *  - 可扩展支持其他键盘输入处理
 */
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

/**
 * @brief 处理鼠标移动事件的回调函数
 * 
 * @param window GLFW 窗口对象
 * @param xpos 鼠标 X 坐标
 * @param ypos 鼠标 Y 坐标
 * 
 * 功能详解:
 *  - 处理鼠标移动，更新摄像机视角
 *  - 首次移动时忽略大幅跳跃
 */
void MouseCallback(GLFWwindow* window, double xpos, double ypos);

/**
 * @brief 处理鼠标滚轮事件的回调函数
 * 
 * @param window GLFW 窗口对象
 * @param xoffset 滚轮 X 偏移
 * @param yoffset 滚轮 Y 偏移
 * 
 * 功能详解:
 *  - 处理鼠标滚轮，调整 FOV（视场角）
 */
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

/**
 * @brief 处理窗口关闭事件
 * 
 * @param window GLFW 窗口对象
 * 
 * 功能详解:
 *  - 检测用户点击窗口关闭按钮
 *  - 优雅地设置退出标志
 */
void WindowCloseCallback(GLFWwindow* window);

/**
 * @brief 处理窗口错误事件的 GLFW 错误回调
 * 
 * @param error_code GLFW 错误代码
 * @param description 错误描述字符串
 * 
 * 功能详解:
 *  - 捕获 GLFW 运行时错误并输出到标准错误流
 *  - 便于快速定位初始化问题
 */
void ErrorCallback(int error_code, const char* description);

/**
 * @brief 清理和释放所有资源
 * 
 * 功能详解:
 *  - 销毁 GLFW 窗口对象
 *  - 终止 GLFW 库
 *  - 释放动态分配的资源
 */
void Cleanup();

/**
 * @brief 主渲染循环 (Game Loop)
 * 
 * 功能详解:
 *  - 循环处理事件、更新和渲染
 *  - 清除帧缓冲并提交交换链
 *  - 通常以 60Hz 或 vsync 限制频率运行
 */
void RenderLoop();

// 全局函数声明
void UpdateWindowTitle();
void EnsureSfxFiles();
void PlaySfxShoot();
void PlaySfxHit();
void PlaySfxKill();

// ============================================================================
// 回调函数实现
// ============================================================================

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // 自适应窗口大小，直接使用当前宽高
    g_windowWidth = std::max(1, width);
    g_windowHeight = std::max(1, height);
    glViewport(0, 0, g_windowWidth, g_windowHeight);
    // 更新鼠标基准，避免尺寸变化导致的大幅跳变
    g_lastX = g_windowWidth / 2.0f;
    g_lastY = g_windowHeight / 2.0f;
    g_resolutionIndex = -1;
    g_resolutionLabel = std::to_string(g_windowWidth) + "x" + std::to_string(g_windowHeight);
    if (g_isPaused) {
        UpdateWindowTitle();
    }
}

void ApplyResolution(int index)
{
    if (index < 0 || index >= RESOLUTION_COUNT) return;
    g_resolutionIndex = index;
    const auto& res = RESOLUTION_OPTIONS[index];
    if (g_window)
    {
        glfwSetWindowSize(g_window, res.width, res.height);
        // reset last mouse positions to center of new window to avoid jump
        g_lastX = res.width / 2.0f;
        g_lastY = res.height / 2.0f;
    }
    g_windowWidth = res.width;
    g_windowHeight = res.height;
    g_resolutionLabel = res.label;
    std::cout << "[Settings] Resolution set to " << res.label << std::endl;
    if (g_isPaused) {
        UpdateWindowTitle();
    }
}

#ifdef _WIN32
// Simple PCM 16-bit mono sine wave writer
void WriteSineWav(const std::string& path, int sampleRate, float freq, float durationSec, float volume)
{
    int totalSamples = static_cast<int>(durationSec * sampleRate);
    int dataSize = totalSamples * 2; // 16-bit mono
    int chunkSize = 36 + dataSize;

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) return;

    auto write32 = [&](uint32_t v){ ofs.write(reinterpret_cast<const char*>(&v), 4); };
    auto write16 = [&](uint16_t v){ ofs.write(reinterpret_cast<const char*>(&v), 2); };

    ofs.write("RIFF", 4);
    write32(chunkSize);
    ofs.write("WAVE", 4);
    ofs.write("fmt ", 4);
    write32(16);           // PCM chunk size
    write16(1);            // PCM
    write16(1);            // Mono
    write32(sampleRate);
    write32(sampleRate * 2); // byte rate
    write16(2);            // block align
    write16(16);           // bits per sample
    ofs.write("data", 4);
    write32(dataSize);

    for (int i = 0; i < totalSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        float sample = std::sin(2.0f * 3.14159265f * freq * t) * volume;
        int16_t s = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f);
        ofs.write(reinterpret_cast<const char*>(&s), 2);
    }
}

void EnsureSfxFiles()
{
    std::filesystem::create_directories("assets");
    if (!std::filesystem::exists(g_sfxShootPath)) WriteSineWav(g_sfxShootPath, 44100, 820.0f, 0.08f, 0.35f);
    if (!std::filesystem::exists(g_sfxHitPath))   WriteSineWav(g_sfxHitPath,   44100, 420.0f, 0.10f, 0.45f);
    if (!std::filesystem::exists(g_sfxKillPath))  WriteSineWav(g_sfxKillPath,  44100, 180.0f, 0.20f, 0.55f);
}

void PlaySfx(const std::string& path)
{
    PlaySoundA(path.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void PlaySfxShoot() { PlaySfx(g_sfxShootPath); }
void PlaySfxHit()   { PlaySfx(g_sfxHitPath); }
void PlaySfxKill()  { PlaySfx(g_sfxKillPath); }
#else
void EnsureSfxFiles() {}
void PlaySfxShoot() {}
void PlaySfxHit() {}
void PlaySfxKill() {}
#endif

void ErrorCallback(int error_code, const char* description)
{
    // 将错误信息输出到标准错误流，便于问题诊断
    std::cerr << "[GLFW ERROR #" << error_code << "]: " << description << std::endl;
}

void KeyCallback([[maybe_unused]] GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods)
{
    // 检测 ESC 键 (暂停/恢复)
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        g_isPaused = !g_isPaused;

        if (g_isPaused)
        {
            std::cout << "[System] Game paused" << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            std::cout << "  [Pause Menu] Controls:" << std::endl;
            std::cout << "  ↑ / ↓ : Adjust mouse sensitivity" << std::endl;
            std::cout << "  ← / → : Adjust field of view (FOV)" << std::endl;
            std::cout << "  ESC   : Resume game" << std::endl;
            std::cout << "  Current resolution: " << g_resolutionLabel << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            
            // 暂停时显示鼠标
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            UpdateWindowTitle(); // 更新标题显示当前设置
        }
        else
        {
            std::cout << "[System] Game resumed" << std::endl;
            // 恢复时隐藏并锁定鼠标
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // 重置首次鼠标标志，防止视角跳跃
            g_firstMouse = true;
            glfwSetWindowTitle(window, WINDOW_TITLE); // 恢复默认标题
        }
    }

}

void UpdateWindowTitle()
{
    if (!g_window) return;
    char title[256];
    const char* resLabel = g_resolutionLabel.c_str();
    sprintf(title, "[Paused] Settings - Sensitivity: %.2f (↑↓) | FOV: %.1f (←→) | Res: %s", 
            g_camera.GetMouseSensitivity(), 
            g_camera.GetFOV(),
            resLabel);
    glfwSetWindowTitle(g_window, title);
}

void MouseCallback([[maybe_unused]] GLFWwindow* window, double xpos, double ypos)
{
    // 暂停时不处理视角移动
    if (g_isPaused) return;

    // 首次鼠标移动时，设置初始位置，避免大幅跳跃
    if (g_firstMouse)
    {
        g_lastX = static_cast<float>(xpos);
        g_lastY = static_cast<float>(ypos);
        g_firstMouse = false;
    }

    // 计算鼠标偏移量
    float xoffset = static_cast<float>(xpos) - g_lastX;
    float yoffset = g_lastY - static_cast<float>(ypos); // 注意 Y 坐标是反的（屏幕坐标从左上角开始）

    // 更新上次鼠标位置
    g_lastX = static_cast<float>(xpos);
    g_lastY = static_cast<float>(ypos);

    // 将鼠标移动传递给摄像机处理
    g_camera.ProcessMouseMovement(xoffset, yoffset);
}

void ScrollCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] double xoffset, double yoffset)
{
    // 将滚轮输入传递给摄像机处理（调整 FOV）
    g_camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void WindowCloseCallback([[maybe_unused]] GLFWwindow* window)
{
    // 用户点击窗口关闭按钮时触发
    g_running = false;
    std::cout << "[Window Event] Close request received, shutting down gracefully..." << std::endl;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // 暂停时不处理射击
    if (g_isPaused) return;

    // 射击逻辑现在移动到了 ProcessInput/RenderLoop 中以支持连发
    // if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    // {
    //     ProcessShooting();
    // }
}

void ProcessShooting()
{
    float currentTime = (float)glfwGetTime();
    if (currentTime - g_lastShootTime < FIRE_RATE) return;
    g_lastShootTime = currentTime;
    PlaySfxShoot();

    // 1. 创建射线
    Ray ray;
    ray.origin = g_camera.GetPosition();
    // 基础方向
    glm::vec3 baseDir = g_camera.GetFront();
    // 添加随机散布
    glm::vec3 right = g_camera.GetRight();
    glm::vec3 up = g_camera.GetUp();
    
    // 生成 [-1, 1] 的随机数
    float r1 = ((float)(rand() % 1000) / 500.0f) - 1.0f;
    float r2 = ((float)(rand() % 1000) / 500.0f) - 1.0f;
    
    // 应用散布
    ray.direction = glm::normalize(baseDir + right * (r1 * SPREAD_AMOUNT) + up * (r2 * SPREAD_AMOUNT));

    // 优化：预计算射线的倒数方向
    // 处理除零问题：如果方向分量接近0，设为一个很大的数
    glm::vec3 invDir;
    invDir.x = (std::abs(ray.direction.x) < 1e-6f) ? 1e20f : 1.0f / ray.direction.x;
    invDir.y = (std::abs(ray.direction.y) < 1e-6f) ? 1e20f : 1.0f / ray.direction.y;
    invDir.z = (std::abs(ray.direction.z) < 1e-6f) ? 1e20f : 1.0f / ray.direction.z;

    // std::cout << "[Shoot] Cast ray! ..." << std::endl; // Removed I/O to reduce stutter
    g_isShooting = true; 

    float closestT = std::numeric_limits<float>::max();
    CubeObject* hitCube = nullptr;
    Enemy* hitEnemy = nullptr;

    // 2. 射线步进 (Voxel Traversal / DDA 简化版)
    glm::vec3 samplePos = ray.origin;
    glm::vec3 step = ray.direction * 0.5f; // 步长 0.5
    float currentDist = 0.0f;
    const float MAX_DIST = 80.0f; // 最大射程

    // 2.1 快速地形检测 (Spatial Grid)
    while (currentDist < MAX_DIST) {
        int x = (int)std::floor(samplePos.x);
        int y = (int)std::floor(samplePos.y);
        int z = (int)std::floor(samplePos.z);
        
        auto* cell = g_spatialGrid.Get(x, y, z);
        if (cell && !cell->empty()) {
            // 这个格子有方块，详细检测 AABB
            for (auto* cube : *cell) {
                glm::vec3 min, max;
                cube->getAABB(min, max);
                
                float t = 0.0f;
                if (intersectRayAABB(ray, invDir, min, max, t)) {
                    if (t < closestT) {
                        closestT = t;
                        hitCube = cube;
                        goto CheckEnemies; // 跳转到敌人检测
                    }
                }
            }
        }
        
        samplePos += step;
        currentDist += 0.5f;
    }

CheckEnemies:
    // 3. 遍历所有敌人
    const auto& activeEnemies = g_enemyPool->GetActiveEnemies();
    for (auto enemy : activeEnemies)
    {
        if (!enemy->IsActive()) continue;

        if (glm::distance(g_camera.GetPosition(), enemy->GetPosition()) > MAX_DIST) continue;

        glm::vec3 min, max;
        enemy->getAABB(min, max);

        float t = 0.0f;
        if (intersectRayAABB(ray, invDir, min, max, t))
        {
            if (t < closestT)
            {
                closestT = t;
                hitEnemy = enemy;
                hitCube = nullptr; // 敌人比地形更近
            }
        }
    }

    // 4. 处理击中反馈
    if (hitCube)
    {
        hitCube->color = glm::vec3(1.0f, 0.0f, 0.0f); // 变红
    }
    else if (hitEnemy)
    {
        bool killed = hitEnemy->TakeDamage(BULLET_DAMAGE); 
        if (killed) PlaySfxKill();
        else PlaySfxHit();
    }

    // 5. 添加子弹轨迹
    glm::vec3 endPoint = ray.origin + ray.direction * (closestT < MAX_DIST ? closestT : MAX_DIST);
    // 从枪口位置(稍微偏移)发射会更真实，这里简化为从摄像机稍微前方发射
    // 为了看清轨迹，起点稍微下移右移一点 (模拟右手持枪)
    glm::vec3 startPoint = ray.origin + g_camera.GetRight() * 0.2f - g_camera.GetUp() * 0.1f + g_camera.GetFront() * 0.5f;
    
    BulletTrail trail;
    trail.start = startPoint;
    trail.end = endPoint;
    trail.timeAlive = 0.0f;
    trail.maxLifetime = 0.1f; // 轨迹持续 0.1 秒
    trail.color = glm::vec4(1.0f, 0.8f, 0.0f, 1.0f); // 金黄色
    g_bulletTrails.push_back(trail);
}

bool intersectRayAABB(const Ray& ray, const glm::vec3& invDir, const glm::vec3& boxMin, const glm::vec3& boxMax, float& t)
{
    // Slab Method 实现 (使用预计算的 invDir)
    glm::vec3 tMin = (boxMin - ray.origin) * invDir;
    glm::vec3 tMax = (boxMax - ray.origin) * invDir;
    
    glm::vec3 t1 = glm::min(tMin, tMax);
    glm::vec3 t2 = glm::max(tMin, tMax);
    
    float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
    float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);
    
    if (tNear > tFar || tFar < 0.0f)
    {
        return false;
    }
    
    t = tNear;
    return true;
}

// ============================================================================
// 初始化函数实现
// ============================================================================

bool InitializeWindow()
{
    std::cout << "[Init] Starting GLFW window setup..." << std::endl;

    // 1. 设置 GLFW 错误回调，以便在初始化过程中捕获错误信息
    glfwSetErrorCallback(ErrorCallback);

    // 2. 初始化 GLFW 库
    if (!glfwInit())
    {
        std::cerr << "[Error] Failed to initialize GLFW!" << std::endl;
        return false;
    }
    std::cout << "[Init] GLFW initialized" << std::endl;

    // Detect desktop resolution and pick 2/3 for default window size
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = primary ? glfwGetVideoMode(primary) : nullptr;
    if (mode)
    {
        g_windowWidth = std::max(640, mode->width * 2 / 3);
        g_windowHeight = std::max(360, mode->height * 2 / 3);
        g_resolutionIndex = -1;
        g_resolutionLabel = std::to_string(g_windowWidth) + "x" + std::to_string(g_windowHeight);
    }
    else
    {
        g_windowWidth = WINDOW_WIDTH;
        g_windowHeight = WINDOW_HEIGHT;
        g_resolutionIndex = 1;
        g_resolutionLabel = RESOLUTION_OPTIONS[g_resolutionIndex].label;
    }

    // 3. 配置 OpenGL 上下文属性
    // 这些提示必须在 glfwCreateWindow 之前设置
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);    // OpenGL 主版本号
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);    // OpenGL 次版本号
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 核心模式
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // 前向兼容性

    // 可选配置：开启 MSAA 抗锯齿 (4x MSAA)
    glfwWindowHint(GLFW_SAMPLES, 4);

    // 4. 创建窗口对象
    g_window = glfwCreateWindow(g_windowWidth, g_windowHeight, WINDOW_TITLE, nullptr, nullptr);
    if (!g_window)
    {
        std::cerr << "[Error] Failed to create window! Check GPU driver and OpenGL support" << std::endl;
        glfwTerminate();
        return false;
    }
    std::cout << "[Init] Window created (" << g_windowWidth << "x" << g_windowHeight << ")" << std::endl;

    // 5. 将 OpenGL 上下文绑定到当前线程
    // 必须在调用任何 OpenGL 函数之前执行
    glfwMakeContextCurrent(g_window);

    // 6. 启用垂直同步 (VSync)
    // 这可以防止画面撕裂，通常以显示器刷新率运行 (60Hz/144Hz 等)
    glfwSwapInterval(1);

    // 7. 注册 GLFW 事件回调函数
    glfwSetFramebufferSizeCallback(g_window, FramebufferSizeCallback); // 注册窗口大小回调
    glfwSetKeyCallback(g_window, KeyCallback);
    glfwSetMouseButtonCallback(g_window, MouseButtonCallback);
    glfwSetWindowCloseCallback(g_window, WindowCloseCallback);
    glfwSetCursorPosCallback(g_window, MouseCallback);
    glfwSetScrollCallback(g_window, ScrollCallback);

    // 8. 隐藏鼠标光标并锁定在窗口中心（第一人称视角模式）
    glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    std::cout << "[Init] Mouse cursor hidden and locked" << std::endl;

    // 9. 设置摄像机参数
    g_camera.SetMovementSpeed(5.0f); // 稍微快一点
    
    // 加载设置
    GameSettings settings = Settings::Load("settings.ini");
    g_camera.SetMouseSensitivity(settings.sensitivity);
    g_camera.SetFOV(settings.fov);
    
    std::cout << "[Init] Camera parameters configured" << std::endl;

    // Sync mouse center with initial window size
    g_lastX = g_windowWidth / 2.0f;
    g_lastY = g_windowHeight / 2.0f;

    std::cout << "[Init] GLFW window initialization completed" << std::endl;
    return true;
}

bool InitializeGLAD()
{
    std::cout << "[Init] Loading OpenGL function pointers..." << std::endl;

    // 1. GLAD 函数加载
    // 必须在 glfwMakeContextCurrent 之后调用
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "[Error] GLAD failed to load OpenGL function pointers!" << std::endl;
        return false;
    }
    std::cout << "[Init] OpenGL function pointers loaded" << std::endl;

    // 2. 输出 OpenGL 版本信息
    const GLubyte* vendor = glGetString(GL_VENDOR);      // 显卡厂商 (e.g., NVIDIA, AMD, Intel)
    const GLubyte* renderer = glGetString(GL_RENDERER);  // 显卡型号
    const GLubyte* version = glGetString(GL_VERSION);    // OpenGL 版本
    const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);  // GLSL 版本

    std::cout << "[GPU Info] Vendor: " << (const char*)vendor << std::endl;
    std::cout << "[GPU Info] Renderer: " << (const char*)renderer << std::endl;
    std::cout << "[GPU Info] OpenGL Version: " << (const char*)version << std::endl;
    std::cout << "[GPU Info] GLSL Version: " << (const char*)glsl_version << std::endl;

    // 3. 验证最小版本要求 (OpenGL 3.3+)
    GLint major_version, minor_version;
    glGetIntegerv(GL_MAJOR_VERSION, &major_version);
    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    
    if (major_version < 3 || (major_version == 3 && minor_version < 3))
    {
        std::cerr << "[Error] GPU does not support OpenGL 3.3+. Current version: " 
                  << major_version << "." << minor_version << std::endl;
        return false;
    }

    // 4. 设置 OpenGL 渲染状态
    glEnable(GL_MULTISAMPLE);  // 启用多重采样抗锯齿
    glEnable(GL_DEPTH_TEST);   // 启用深度测试

    std::cout << "[Init] GLAD initialization complete" << std::endl;
    return true;
}

bool InitializeScene()
{
    std::cout << "[Init] Building scene..." << std::endl;
    EnsureSfxFiles();

    // 1. 加载着色器
    g_shader = new Shader("shaders/phong.vert", "shaders/phong.frag");
    g_instancedShader = new Shader("shaders/instanced.vert", "shaders/instanced.frag"); // 加载实例化着色器
    if (g_shader->ID == 0 || g_instancedShader->ID == 0) return false;

    // 获取原始数据以创建 InstancedMesh
    Geometry::MeshData cubeData = Geometry::createCubeData(1.0f);
    
    // 创建普通 Mesh 用于其他物体
    g_cubeMesh = new Mesh(cubeData.vertices, cubeData.indices); 
    
    // 使用相同的数据创建地形 InstancedMesh
    g_terrainMesh = new InstancedMesh(cubeData.vertices, cubeData.indices);


    // 3. 生成随机 Minecraft 风格地图 (使用 Perlin Noise)
    Perlin2D perlin(12345);
    TerrainParams tp;
    
    // 生成地形 (64x64 区域)
    const int mapSize = 64; 
    
    std::vector<glm::vec3> instancePositions;
    std::vector<glm::vec3> instanceColors;
    g_terrainPositions.clear(); // 清空物理数据
    g_cubes.clear(); // ！！！重要：清空旧的立方体数据，重新填充
    g_spatialGrid.Clear(); // 清空空间网格
    
    std::mt19937 rng(std::random_device{}());
    
    for (int x = -mapSize; x < mapSize; ++x)
    {
        for (int z = -mapSize; z < mapSize; ++z)
        {
            float h = terrainHeight(perlin, tp, x, z);
            int height = static_cast<int>(std::floor(h));
            
            // 填充从底部到地表
            int bottomY = height - 4; 
            if (bottomY < -10) bottomY = -10;

            for (int y = bottomY; y <= height; ++y)
            {
                BlockType type = BlockType::Stone;
                
                // 简单的生物群落规则
                if (y == height) {
                    if (y < tp.waterLevel) type = BlockType::Sand; 
                    else if (y < tp.waterLevel + tp.beachHeight) type = BlockType::Sand;
                    else if (y > tp.snowHeight) type = BlockType::Snow;
                    else type = BlockType::Grass;
                } else if (y > height - 3) {
                    type = BlockType::Dirt;
                }

                glm::vec3 pos(x * 1.0f, y * 1.0f, z * 1.0f);
                glm::vec3 color = getBlockColor(type);

                instancePositions.push_back(pos);
                g_terrainPositions.push_back(pos);
                instanceColors.push_back(color);
                
                // ！！！添加 CubeObject 到 g_cubes，修复灰色屏幕和射击问题
                // 注意：vector 扩容会导致指针失效，所以我们最好先 reserve，或者用 list，或者最后再构建 grid。
                // 为了避免指针失效，我们修改 CubeObject 的存储方式？
                // 或者简单点：我们先生成完所有 g_cubes，然后再构建 SpatialGrid。
                CubeObject cube;
                cube.position = pos;
                cube.scale = glm::vec3(1.0f);
                cube.color = color;
                g_cubes.push_back(cube);
                
                // 树木生成 (只在草地上)
                if (y == height && type == BlockType::Grass) {
                    float randVal = (float)(rng() % 1000) / 1000.0f;
                    if (randVal > tp.treeThreshold) {
                        int treeHeight = 4 + (rng() % 3);
                        // 树干
                        for (int th = 1; th <= treeHeight; ++th) {
                            glm::vec3 tPos = pos + glm::vec3(0.0f, (float)th, 0.0f);
                            glm::vec3 tColor = getBlockColor(BlockType::Wood);
                            instancePositions.push_back(tPos);
                            g_terrainPositions.push_back(tPos); 
                            instanceColors.push_back(tColor);

                            CubeObject tCube;
                            tCube.position = tPos;
                            tCube.scale = glm::vec3(1.0f);
                            tCube.color = tColor;
                            g_cubes.push_back(tCube);
                        }
                        // 简化树叶：只在树顶加个 3x3x2 的盖子
                        for(int lx=-1; lx<=1; ++lx) {
                            for(int lz=-1; lz<=1; ++lz) {
                                for(int ly=0; ly<=1; ++ly) {
                                    glm::vec3 lPos = pos + glm::vec3((float)lx, (float)treeHeight + (float)ly, (float)lz);
                                    if (lx==0 && lz==0 && ly==0) continue; // 树干位置
                                    glm::vec3 lColor = getBlockColor(BlockType::Leaves);
                                    instancePositions.push_back(lPos);
                                    instanceColors.push_back(lColor);

                                    CubeObject lCube;
                                    lCube.position = lPos;
                                    lCube.scale = glm::vec3(1.0f);
                                    lCube.color = lColor;
                                    g_cubes.push_back(lCube);
                                }
                            }
                        }
                    }
                }
            }
            
            // 水面填充
            if (height < tp.waterLevel) {
                for (int y = height + 1; y <= (int)tp.waterLevel; ++y) {
                    glm::vec3 pos(x * 1.0f, y * 1.0f, z * 1.0f);
                    glm::vec3 color = getBlockColor(BlockType::Water);
                    instancePositions.push_back(pos);
                    instanceColors.push_back(color);

                    CubeObject wCube;
                    wCube.position = pos;
                    wCube.scale = glm::vec3(1.0f);
                    wCube.color = color;
                    g_cubes.push_back(wCube);
                }
            }
        }
    }

    
    // 上传数据到 GPU
    g_terrainMesh->updateInstanceData(instancePositions, instanceColors);
    
    // 构建空间网格 (在 g_cubes 填充完毕后)
    for (auto& cube : g_cubes) {
        g_spatialGrid.Add(&cube);
    }
    
    std::cout << "[Init] Terrain generated. Block count: " << instancePositions.size() << std::endl;

    // 4. 调整摄像机高度以防出生在地底
    // 寻找 (0,0) 附近的最高点
    float spawnY = 5.0f; // 默认高度
    for (const auto& pos : g_terrainPositions) {
        if (std::abs(pos.x - 0.0f) < 1.0f && std::abs(pos.z - 0.0f) < 1.0f) {
            if (pos.y > spawnY) spawnY = pos.y;
        }
    }
    // 设置摄像机位置 (在最高点上方 2.0f)
    g_camera.SetPosition(glm::vec3(0.0f, spawnY + 2.0f, 0.0f));
    std::cout << "[Init] Adjusted spawn height: " << spawnY + 2.0f << std::endl;

    // 初始化准星
    g_crosshairShader = new Shader("shaders/crosshair.vert", "shaders/crosshair.frag");
    float crosshairVertices[] = {
        // 横线
        -0.02f, 0.0f,
         0.02f, 0.0f,
        // 竖线 
         0.0f, -0.03f,
         0.0f,  0.03f
    };
    glGenVertexArrays(1, &g_crosshairVAO);
    glGenBuffers(1, &g_crosshairVBO);
    glBindVertexArray(g_crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // 初始化 UI Quad (用于进度条)
    float quadVertices[] = {
        // Pos (0,0 to 1,1) - Triangle Strip
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };
    glGenVertexArrays(1, &g_uiVAO);
    glGenBuffers(1, &g_uiVBO);
    glBindVertexArray(g_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // 10. 初始化子弹轨迹渲染资源
    g_lineShader = new Shader("shaders/line.vert", "shaders/line.frag");
    glGenVertexArrays(1, &g_lineVAO);
    glGenBuffers(1, &g_lineVBO);
    // VBO 数据在每帧更新，先不绑定数据
    glBindVertexArray(g_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_lineVBO);
    // 两个属性: pos(vec3), color(vec4) -> stride = 7 * float
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // 4. 初始化 AI 系统
    g_enemyPool = new EnemyPool(100); // 初始池大小 100
    g_director = new AIDirector(g_enemyPool);

    std::cout << "[Init] Scene build complete" << std::endl;
    return true;
}

// 移除旧的 SpawnEnemies 函数，现在由 AIDirector 接管

// ============================================================================
// 清理函数实现
// ============================================================================

void Cleanup()
{
    std::cout << "[Cleanup] Releasing system resources..." << std::endl;

    delete g_shader;
    delete g_instancedShader;
    delete g_cubeMesh;
    // delete g_planeMesh;
    
    delete g_director;
    delete g_enemyPool;
    delete g_terrainMesh; // 记得删除
    delete g_crosshairShader;
    delete g_lineShader; // 删除 LineShader
    glDeleteVertexArrays(1, &g_crosshairVAO);
    glDeleteBuffers(1, &g_crosshairVBO);
    glDeleteVertexArrays(1, &g_lineVAO); // 删除 Line VAO
    glDeleteBuffers(1, &g_lineVBO); // 删除 Line VBO
    glDeleteVertexArrays(1, &g_uiVAO);
    glDeleteBuffers(1, &g_uiVBO);

    // 保存设置
    GameSettings settings;
    settings.sensitivity = g_camera.GetMouseSensitivity();
    settings.fov = g_camera.GetFOV();
    Settings::Save("settings.ini", settings);

    if (g_window)
    {
        glfwDestroyWindow(g_window);
        std::cout << "[Cleanup] Window destroyed" << std::endl;
    }

    glfwTerminate();
    std::cout << "[Cleanup] GLFW terminated" << std::endl;
    std::cout << "[Cleanup] Cleanup finished. Exiting application" << std::endl;
}

// ============================================================================
// 主循环函数实现
// ============================================================================

void RenderLoop()
{
    std::cout << "[Loop] Entering main render loop..." << std::endl;
    std::cout << "[Tip] Use WASD to move, mouse to look, ESC to exit" << std::endl;

    // 主事件循环：处理窗口事件、更新逻辑、渲染画面
    while (g_running && !glfwWindowShouldClose(g_window))
    {
        // -------- 时间计算阶段 --------
        // 计算当前帧时间
        float currentFrame = static_cast<float>(glfwGetTime());
        g_deltaTime = currentFrame - g_lastFrame;
        g_lastFrame = currentFrame;
        // Clamp delta time to avoid physics tunneling during window resize stalls
        g_deltaTime = std::min(g_deltaTime, 0.05f);

        // -------- 事件处理阶段 --------
        // 处理所有待处理的窗口事件 (键盘、鼠标、窗口大小调整等)
        glfwPollEvents();

        // -------- 输入处理阶段 --------
        // 处理键盘输入（WASD 移动）
        if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS)
            g_camera.ProcessKeyboard(Camera::Movement::FORWARD, g_deltaTime);
        if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS)
            g_camera.ProcessKeyboard(Camera::Movement::BACKWARD, g_deltaTime);
        if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS)
            g_camera.ProcessKeyboard(Camera::Movement::LEFT, g_deltaTime);
        if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS)
            g_camera.ProcessKeyboard(Camera::Movement::RIGHT, g_deltaTime);
        
        // 跳跃输入
        if (glfwGetKey(g_window, GLFW_KEY_SPACE) == GLFW_PRESS)
            g_camera.ProcessJump();
        
        // 射击输入 (连发)
        if (!g_isPaused && glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            ProcessShooting();
        }
            
        // 物理更新
        g_camera.UpdatePhysics(g_deltaTime, g_terrainPositions);


        // -------- 清除缓冲区阶段 --------
        // 清除颜色缓冲和深度缓冲
        // glClear() 使用 glClearColor() 设置的颜色填充帧缓冲
        glClearColor(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // -------- 渲染阶段 --------
        // 激活着色器
        g_shader->use();

        // 获取摄像机的观察矩阵和投影矩阵
        glm::mat4 view = g_camera.GetViewMatrix();
        // 投影矩阵使用当前窗口宽高比，支持任意窗口拉伸
        float aspect = (g_windowHeight > 0) ? (static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight)) : (16.0f / 9.0f);
        glm::mat4 projection = g_camera.GetProjectionMatrix(
            g_camera.GetFOV(),                        // FOV (动态获取)
            aspect,                                   // 当前窗口宽高比
            0.1f,                                     // 近裁剪面
            100.0f                                    // 远裁剪面
        );

        // 设置全局 Uniforms
        g_shader->setMat4("uView", view);
        g_shader->setMat4("uProjection", projection);
        g_shader->setVec3("uCameraPos", g_camera.GetPosition());

        // 设置光源属性 (一个静态的点光源，提高位置以照亮山顶)
        glm::vec3 lightPos(20.0f, 100.0f, 20.0f); // 模拟太阳高度
        g_shader->setVec3("uLight_Position", lightPos);
        g_shader->setVec3("uLight_Ambient", glm::vec3(0.3f, 0.3f, 0.3f)); // 稍微提高环境光
        g_shader->setVec3("uLight_Diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
        g_shader->setVec3("uLight_Specular", glm::vec3(1.0f, 1.0f, 1.0f));

        // 1. 渲染地面
        {
            glm::mat4 model = glm::mat4(1.0f); // 单位矩阵
            g_shader->setMat4("uModel", model);
            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
            g_shader->setMat3("uNormalMatrix", normalMatrix);

            // 地面材质 (深灰色，低反光)
            g_shader->setVec3("uMaterial_Ambient", glm::vec3(0.1f, 0.1f, 0.1f));
            g_shader->setVec3("uMaterial_Diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
            g_shader->setVec3("uMaterial_Specular", glm::vec3(0.1f, 0.1f, 0.1f));
            g_shader->setFloat("uMaterial_Shininess", 8.0f);

            // g_planeMesh->draw(); // 不再绘制平面，使用生成的体素地图
        }

        // 2. 渲染立方体 (地形)
        {
            // 切换到实例化着色器
            g_instancedShader->use();
            
            // 设置公共 Uniforms (View, Proj, Light)
            g_instancedShader->setMat4("uView", view);
            g_instancedShader->setMat4("uProjection", projection);
            g_instancedShader->setVec3("uCameraPos", g_camera.GetPosition());
            
            g_instancedShader->setVec3("uLight_Position", lightPos);
            g_instancedShader->setVec3("uLight_Ambient", glm::vec3(0.3f, 0.3f, 0.3f));
            g_instancedShader->setVec3("uLight_Diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
            g_instancedShader->setVec3("uLight_Specular", glm::vec3(1.0f, 1.0f, 1.0f));

            g_instancedShader->setVec3("uMaterial_Ambient", glm::vec3(0.1f, 0.1f, 0.1f)); 
            g_instancedShader->setVec3("uMaterial_Specular", glm::vec3(0.1f, 0.1f, 0.1f));
            g_instancedShader->setFloat("uMaterial_Shininess", 8.0f);
            
            // 绘制调用：只需要一次！
            g_terrainMesh->drawInstanced(static_cast<unsigned int>(g_cubes.size()));
        }

        // 切换回标准着色器绘制其他物体
        g_shader->use();
        g_shader->setMat4("uView", view);
        g_shader->setMat4("uProjection", projection);
        g_shader->setVec3("uCameraPos", g_camera.GetPosition());
        g_shader->setVec3("uLight_Position", lightPos);
        // ... (其他光照参数其实可以复用，或者封装成函数 SetLightUniforms)
        g_shader->setVec3("uLight_Ambient", glm::vec3(0.3f, 0.3f, 0.3f));
        g_shader->setVec3("uLight_Diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
        g_shader->setVec3("uLight_Specular", glm::vec3(1.0f, 1.0f, 1.0f));

        // 3. 更新并渲染敌人
        {
            // 如果没暂停，才更新逻辑
            if (!g_isPaused) {
                g_director->Update(g_deltaTime, g_isShooting);
                g_isShooting = false; 
                glm::vec3 playerPos = g_camera.GetPosition();
                g_enemyPool->UpdateAll(g_deltaTime, playerPos, g_terrainPositions);
            }
            else 
            {
                // 暂停时的设置逻辑 (处理连续按键)
                bool changed = false;
                float sens = g_camera.GetMouseSensitivity();
                float fov = g_camera.GetFOV();

                // 灵敏度调整
                if (glfwGetKey(g_window, GLFW_KEY_UP) == GLFW_PRESS) {
                    sens += 0.1f * g_deltaTime;
                    changed = true;
                }
                if (glfwGetKey(g_window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                    sens -= 0.1f * g_deltaTime;
                    if (sens < 0.01f) sens = 0.01f;
                    changed = true;
                }
                
                // FOV 调整
                if (glfwGetKey(g_window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                    fov += 30.0f * g_deltaTime;
                    changed = true;
                }
                if (glfwGetKey(g_window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                    fov -= 30.0f * g_deltaTime;
                    changed = true;
                }

                if (changed) {
                    g_camera.SetMouseSensitivity(sens);
                    g_camera.SetFOV(fov);
                    UpdateWindowTitle();
                }
            }

            g_shader->setVec3("uMaterial_Specular", glm::vec3(0.1f, 0.1f, 0.1f)); 
            g_shader->setFloat("uMaterial_Shininess", 4.0f);

            // 获取活跃敌人列表进行渲染
            const auto& activeEnemies = g_enemyPool->GetActiveEnemies();
            for (auto enemy : activeEnemies)
            {
                // 渲染 (即使是尸体也渲染，直到被回收)
                g_shader->setVec3("uMaterial_Diffuse", enemy->GetColor());

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, enemy->GetPosition());
                
                // 应用旋转 (包含倒地动画)
                model *= glm::mat4_cast(enemy->GetRotation());
                
                model = glm::scale(model, enemy->GetScale());
                
                g_shader->setMat4("uModel", model);
                
                glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
                g_shader->setMat3("uNormalMatrix", normalMatrix);

                g_cubeMesh->draw();
            }
        }

#if 0 // Disabled enemy health bars
        // 3.1 敌人血条（屏幕空间绘制）
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            g_crosshairShader->use();
            g_crosshairShader->setFloat("uAlpha", 0.65f);
            glBindVertexArray(g_uiVAO);

            auto DrawRect = [&](float x, float y, float w, float h, glm::vec3 color) {
                if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || !std::isfinite(h)) return;
                g_crosshairShader->setVec3("uColor", color);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(x, y, 0.0f));
                model = glm::scale(model, glm::vec3(w, h, 1.0f));
                g_crosshairShader->setMat4("uModel", model);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            };

            const auto& activeEnemies = g_enemyPool->GetActiveEnemies();
            for (auto enemy : activeEnemies)
            {
                if (!enemy->IsActive()) continue;
                glm::vec3 headPos = enemy->GetPosition() + glm::vec3(0.0f, enemy->GetScale().y * 0.6f + 0.4f, 0.0f);
                glm::vec4 clip = projection * view * glm::vec4(headPos, 1.0f);
                if (clip.w <= 0.0f) continue;
                glm::vec3 ndc = glm::vec3(clip) / clip.w;
                if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y)) continue;
                if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f) continue;
                if (ndc.z < -1.0f || ndc.z > 1.0f) continue;

                float winW = static_cast<float>(std::max(1, g_windowWidth));
                float winH = static_cast<float>(std::max(1, g_windowHeight));
                float barWidth = 40.0f / winW * 2.0f;   // 40px
                float barHeight = 6.0f / winH * 2.0f;   // 6px
                float pad = 2.0f / winW * 2.0f;         // 2px (x)
                float startX = ndc.x - barWidth * 0.5f;
                float startY = ndc.y + (24.0f / winH * 2.0f); // 24px above
                float hp = glm::clamp(enemy->GetHealth(), 0.0f, ENEMY_MAX_HEALTH);
                float ratio = hp / ENEMY_MAX_HEALTH;
                glm::vec3 bgColor(0.08f, 0.08f, 0.08f);
                glm::vec3 hpColor = (ratio > 0.5f) ? glm::vec3(0.25f, 0.8f, 0.25f) : glm::vec3(0.95f, 0.35f, 0.1f);

                DrawRect(startX, startY, barWidth, barHeight, bgColor);
                DrawRect(startX + pad, startY + pad, (barWidth - pad * 2.0f) * ratio, barHeight - pad * 2.0f, hpColor);
            }

            g_crosshairShader->setFloat("uAlpha", 1.0f);
            glDisable(GL_BLEND);
            glBindVertexArray(0);
            glUseProgram(0);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
        }
#endif

        // 4. 渲染武器 (右下角小尺寸，避免遮挡视野)
        {
            glEnable(GL_DEPTH_TEST);

            glm::mat4 viewIdentity = glm::mat4(1.0f);
            g_shader->setMat4("uView", viewIdentity);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.5f, -0.5f, -0.7f)); 
            model = glm::scale(model, glm::vec3(0.018f, 0.035f, 0.22f));
            model = glm::rotate(model, glm::radians(12.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            g_shader->setMat4("uModel", model);
            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
            g_shader->setMat3("uNormalMatrix", normalMatrix);

            g_shader->setVec3("uMaterial_Diffuse", glm::vec3(0.2f, 0.2f, 0.25f));
            g_shader->setVec3("uMaterial_Specular", glm::vec3(0.3f, 0.3f, 0.3f));
            g_shader->setFloat("uMaterial_Shininess", 24.0f);
            g_shader->setVec3("uCameraPos", glm::vec3(0.0f)); 
            g_cubeMesh->draw();
        }

        // 渲染子弹轨迹 (透明混合)
        if (!g_bulletTrails.empty()) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            g_lineShader->use();
            g_lineShader->setMat4("uProjection", projection);
            g_lineShader->setMat4("uView", view);
            
            std::vector<float> lineData;
            for (auto it = g_bulletTrails.begin(); it != g_bulletTrails.end(); ) {
                it->timeAlive += g_deltaTime;
                if (it->timeAlive >= it->maxLifetime) {
                    it = g_bulletTrails.erase(it);
                    continue;
                }
                
                // 渐变透明度
                float alpha = 1.0f - (it->timeAlive / it->maxLifetime);
                
                // Start Point
                lineData.push_back(it->start.x);
                lineData.push_back(it->start.y);
                lineData.push_back(it->start.z);
                lineData.push_back(it->color.r);
                lineData.push_back(it->color.g);
                lineData.push_back(it->color.b);
                lineData.push_back(alpha);
                
                // End Point
                lineData.push_back(it->end.x);
                lineData.push_back(it->end.y);
                lineData.push_back(it->end.z);
                lineData.push_back(it->color.r);
                lineData.push_back(it->color.g);
                lineData.push_back(it->color.b);
                lineData.push_back(alpha); // 尾部也可以 fade，或者 0.0
                
                ++it;
            }
            
            if (!lineData.empty()) {
                glBindVertexArray(g_lineVAO);
                glBindBuffer(GL_ARRAY_BUFFER, g_lineVBO);
                // 使用 glBufferData 重新分配并上传数据
                glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);
                
                // 正确的顶点数量是 float 总数 / 7
                glDrawArrays(GL_LINES, 0, (GLsizei)lineData.size() / 7);
                glBindVertexArray(0);
            }
            
            glDisable(GL_BLEND);
        }

        // 5. 绘制准星 (UI 层，最后绘制，关闭深度测试)
        if (!g_isPaused)
        {
            glDisable(GL_DEPTH_TEST); // 关闭深度测试，确保准星在最上层
            g_crosshairShader->use();
            g_crosshairShader->setFloat("uAlpha", 1.0f);
            g_crosshairShader->setVec3("uColor", glm::vec3(0.0f, 1.0f, 0.0f)); // 绿色准星
            
            // 准星不需要模型变换，或者设置为单位矩阵
            g_crosshairShader->setMat4("uModel", glm::mat4(1.0f));

            glBindVertexArray(g_crosshairVAO);
            glDrawArrays(GL_LINES, 0, 4); // 绘制十字线
            
            glEnable(GL_DEPTH_TEST); // 恢复深度测试
        }
        else
        {
            // 绘制暂停菜单 UI (进度条)
            glDisable(GL_DEPTH_TEST);
            g_crosshairShader->use(); // 复用这个简单的 2D Shader
            g_crosshairShader->setFloat("uAlpha", 0.4f); // 半透明，避免遮挡视野

            // 辅助 Lambda: 绘制矩形 (x,y 是左下角, w,h 是宽高, color 是颜色)
            auto DrawRect = [&](float x, float y, float w, float h, glm::vec3 color) {
                g_crosshairShader->setVec3("uColor", color);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(x, y, 0.0f));
                model = glm::scale(model, glm::vec3(w, h, 1.0f));
                g_crosshairShader->setMat4("uModel", model);
                glBindVertexArray(g_uiVAO);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            };

            // 辅助 Lambda: 绘制数字 (7段数码管风格)
            auto DrawDigit = [&](int digit, float x, float y, float size, glm::vec3 color) {
                // 7段定义: 0-6 (A, B, C, D, E, F, G)
                //   A
                // F   B
                //   G
                // E   C
                //   D
                bool segs[10][7] = {
                    {1,1,1,1,1,1,0}, // 0
                    {0,1,1,0,0,0,0}, // 1
                    {1,1,0,1,1,0,1}, // 2
                    {1,1,1,1,0,0,1}, // 3
                    {0,1,1,0,0,1,1}, // 4
                    {1,0,1,1,0,1,1}, // 5
                    {1,0,1,1,1,1,1}, // 6
                    {1,1,1,0,0,0,0}, // 7
                    {1,1,1,1,1,1,1}, // 8
                    {1,1,1,1,0,1,1}  // 9
                };
                
                float t = size * 0.1f; // 厚度
                float l = size;        // 长度
                
                // 段位置 offset
                if(segs[digit][0]) DrawRect(x, y + 2*l, l, t, color); // A
                if(segs[digit][1]) DrawRect(x + l, y + l, t, l, color); // B
                if(segs[digit][2]) DrawRect(x + l, y, t, l, color); // C
                if(segs[digit][3]) DrawRect(x, y, l, t, color); // D
                if(segs[digit][4]) DrawRect(x - t, y, t, l, color); // E
                if(segs[digit][5]) DrawRect(x - t, y + l, t, l, color); // F
                if(segs[digit][6]) DrawRect(x, y + l, l, t, color); // G
            };

            // 辅助 Lambda: 绘制浮点数 (格式: X.XX)
            auto DrawFloat = [&](float value, float x, float y, float size, glm::vec3 color) {
                int intPart = (int)value;
                int fracPart = (int)((value - intPart) * 100); // 两位小数
                
                // 绘制整数部分 (假设最多3位)
                float cursorX = x;
                if (intPart >= 100) { DrawDigit(intPart / 100, cursorX, y, size, color); cursorX += size * 1.5f; }
                if (intPart >= 10)  { DrawDigit((intPart / 10) % 10, cursorX, y, size, color); cursorX += size * 1.5f; }
                DrawDigit(intPart % 10, cursorX, y, size, color); cursorX += size * 1.5f;
                
                // 小数点
                DrawRect(cursorX, y, size*0.2f, size*0.2f, color); cursorX += size * 0.5f;
                
                // 小数部分
                DrawDigit(fracPart / 10, cursorX, y, size, color); cursorX += size * 1.5f;
                DrawDigit(fracPart % 10, cursorX, y, size, color);
            };

            // 1. 灵敏度进度条 (位置 y=0.2)
            float sens = g_camera.GetMouseSensitivity();
            float sensProgress = glm::clamp((sens - 0.01f) / (1.0f - 0.01f), 0.0f, 1.0f); // 假设最大灵敏度 1.0
            
            float barW = 0.25f;
            float barH = 0.02f;
            float barX = -barW * 0.5f;
            float barY = 0.25f;

            // 背景 (深灰)
            DrawRect(barX, barY, barW, barH, glm::vec3(0.15f, 0.15f, 0.15f));
            // 进度 (绿色)
            DrawRect(barX, barY, barW * sensProgress, barH, glm::vec3(0.2f, 0.8f, 0.2f));
            // 数值显示（更小字号，靠近右侧）
            DrawFloat(sens, barX + barW + 0.05f, barY, 0.02f, glm::vec3(0.2f, 0.8f, 0.2f));

            // 2. FOV 进度条 (位置 y=-0.2)
            float fov = g_camera.GetFOV();
            float fovProgress = glm::clamp((fov - 10.0f) / (120.0f - 10.0f), 0.0f, 1.0f);

            float fbarY = -0.25f;
            // 背景
            DrawRect(barX, fbarY, barW, barH, glm::vec3(0.15f, 0.15f, 0.15f));
            // 进度 (蓝色)
            DrawRect(barX, fbarY, barW * fovProgress, barH, glm::vec3(0.2f, 0.2f, 0.8f));
            // 数值显示
            DrawFloat(fov, barX + barW + 0.05f, fbarY, 0.02f, glm::vec3(0.2f, 0.2f, 0.8f));

            g_crosshairShader->setFloat("uAlpha", 1.0f); // 恢复
            glEnable(GL_DEPTH_TEST);
        }

        // -------- 缓冲区交换阶段 --------
        // 交换前后缓冲区 (双缓冲)，将渲染结果显示到屏幕
        // 这可以防止画面闪烁
        glfwSwapBuffers(g_window);
    }

    std::cout << "[Loop] Exited main render loop" << std::endl;
}

// ============================================================================
// 主程序入口
// ============================================================================

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
#ifdef _WIN32
    // 设置控制台代码页为 UTF-8，解决乱码问题
    SetConsoleOutputCP(65001);
#endif

    std::cout << "===========================================================" << std::endl;
    std::cout << "  OpenGL baseline renderer starting" << std::endl;
    std::cout << "  Standard: C++17 | Display: OpenGL 4.6 Core" << std::endl;
    std::cout << "===========================================================" << std::endl;

    // -------- 初始化阶段 --------
    try
    {
        // 初始化 GLFW 并创建窗口
        if (!InitializeWindow())
        {
            std::cerr << "[Fatal] Failed to initialize GLFW window!" << std::endl;
            Cleanup();
            return EXIT_FAILURE;
        }

        // 初始化 GLAD 并加载 OpenGL 函数指针
        if (!InitializeGLAD())
        {
            std::cerr << "[Fatal] GLAD initialization failed!" << std::endl;
            Cleanup();
            return EXIT_FAILURE;
        }

        // 初始化场景
        if (!InitializeScene())
        {
            std::cerr << "[Fatal] Scene initialization failed!" << std::endl;
            Cleanup();
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Exception] Unexpected error during initialization: " << e.what() << std::endl;
        Cleanup();
        return EXIT_FAILURE;
    }

    // -------- 主循环阶段 --------
    RenderLoop();

    // -------- 清理阶段 --------
    Cleanup();

    std::cout << "===========================================================" << std::endl;
    std::cout << "  Application exited normally" << std::endl;
    std::cout << "===========================================================" << std::endl;

    return EXIT_SUCCESS;
}

