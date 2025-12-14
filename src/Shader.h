// ============================================================================
// Shader.h - 现代OpenGL着色器程序封装类
// 功能: 加载、编译、链接着色器程序，提供uniform变量设置接口
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * @class Shader
 * @brief 现代OpenGL着色器程序封装类
 * @details 
 *   - 支持从文件或字符串加载着色器源码
 *   - 自动编译和链接着色器程序
 *   - 提供完善的错误检测和日志输出
 *   - 支持设置各类uniform变量（float, int, mat4, vec3等）
 *   - 使用OpenGL 3.3+ 核心模式
 */
class Shader
{
public:
    // 着色器程序的OpenGL ID
    GLuint ID;

    /**
     * @brief 构造函数 - 从文件加载着色器
     * @param vertexPath 顶点着色器文件路径
     * @param fragmentPath 片段着色器文件路径
     * @param geometryPath 几何着色器文件路径（可选）
     */
    Shader(const std::string& vertexPath, 
           const std::string& fragmentPath,
           const std::string& geometryPath = "");

    /**
     * @brief 默认构造函数
     */
    Shader() : ID(0) {}

    /**
     * @brief 静态方法 - 从字符串加载着色器源码
     * @param vertexCode 顶点着色器源码
     * @param fragmentCode 片段着色器源码
     * @param geometryCode 几何着色器源码（可选）
     */
    static Shader fromSource(const std::string& vertexCode,
           const std::string& fragmentCode,
           const std::string& geometryCode = "");

    ~Shader() = default;

    /**
     * @brief 激活着色器程序
     */
    void use() const;

    /**
     * @brief 设置布尔uniform变量
     */
    void setBool(const std::string& name, bool value) const;

    /**
     * @brief 设置整数uniform变量
     */
    void setInt(const std::string& name, int value) const;

    /**
     * @brief 设置浮点数uniform变量
     */
    void setFloat(const std::string& name, float value) const;

    /**
     * @brief 设置vec3 uniform变量
     */
    void setVec3(const std::string& name, const glm::vec3& value) const;

    /**
     * @brief 设置vec3 uniform变量（三个浮点数）
     */
    void setVec3(const std::string& name, float x, float y, float z) const;

    /**
     * @brief 设置vec4 uniform变量
     */
    void setVec4(const std::string& name, const glm::vec4& value) const;

    /**
     * @brief 设置mat4 uniform变量
     */
    void setMat4(const std::string& name, const glm::mat4& mat) const;

    /**
     * @brief 设置mat3 uniform变量
     */
    void setMat3(const std::string& name, const glm::mat3& mat) const;

private:
    /**
     * @brief 从文件读取着色器源码
     * @param filePath 文件路径
     * @return 着色器源码字符串
     */
    static std::string readShaderFile(const std::string& filePath);

    /**
     * @brief 编译单个着色器
     * @param type 着色器类型（GL_VERTEX_SHADER, GL_FRAGMENT_SHADER等）
     * @param source 着色器源码
     * @return 编译后的着色器ID
     */
    static GLuint compileShader(GLenum type, const std::string& source);

    /**
     * @brief 链接着色器程序
     * @param vertexShader 顶点着色器ID
     * @param fragmentShader 片段着色器ID
     * @param geometryShader 几何着色器ID（可选，传0表示无）
     * @return 链接后的程序ID
     */
    static GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint geometryShader = 0);

    /**
     * @brief 获取uniform变量位置
     * @param name uniform变量名称
     * @return uniform变量位置
     */
    GLint getUniformLocation(const std::string& name) const;
};

