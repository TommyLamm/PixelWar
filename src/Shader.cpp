// ============================================================================
// Shader.cpp - 现代OpenGL着色器程序封装类实现
// 功能: 加载、编译、链接着色器程序，提供uniform变量设置接口
// 作者: 游戏引擎架构组
// 标准: C++17
// ============================================================================

#include "Shader.h"

// 从文件路径构造
Shader::Shader(const std::string& vertexPath, 
               const std::string& fragmentPath,
               const std::string& geometryPath)
{
    try
    {
        // 读取着色器源码
        std::string vertexCode = readShaderFile(vertexPath);
        std::string fragmentCode = readShaderFile(fragmentPath);
        std::string geometryCode;

        if (!geometryPath.empty())
        {
            geometryCode = readShaderFile(geometryPath);
        }

        // 编译着色器
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode);
        GLuint geometryShader = 0;

        if (!geometryPath.empty())
        {
            geometryShader = compileShader(GL_GEOMETRY_SHADER, geometryCode);
        }

        // 链接程序
        ID = linkProgram(vertexShader, fragmentShader, geometryShader);

        // 删除着色器对象（已链接到程序中）
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader != 0)
        {
            glDeleteShader(geometryShader);
        }

        std::cout << "[Shader] Program loaded successfully. ID: " << ID << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Shader Error] " << e.what() << std::endl;
        ID = 0;
    }
}

// 从字符串构造
Shader Shader::fromSource(const std::string& vertexCode,
               const std::string& fragmentCode,
               const std::string& geometryCode)
{
    Shader shader;
    try
    {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode);
        GLuint geometryShader = 0;

        if (!geometryCode.empty())
        {
            geometryShader = compileShader(GL_GEOMETRY_SHADER, geometryCode);
        }

        shader.ID = linkProgram(vertexShader, fragmentShader, geometryShader);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader != 0)
        {
            glDeleteShader(geometryShader);
        }

        std::cout << "[Shader] Program (from source) loaded successfully. ID: " << shader.ID << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Shader Error] " << e.what() << std::endl;
        shader.ID = 0;
    }
    return shader;
}

std::string Shader::readShaderFile(const std::string& filePath)
{
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        throw std::runtime_error("Failed to read shader file: " + filePath + "\nDetails: " + std::string(e.what()));
    }
}

GLuint Shader::compileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* cSource = source.c_str();
    glShaderSource(shader, 1, &cSource, nullptr);
    glCompileShader(shader);

    // 检查编译错误
    int success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::string shaderType;
        switch (type)
        {
            case GL_VERTEX_SHADER: shaderType = "Vertex shader"; break;
            case GL_FRAGMENT_SHADER: shaderType = "Fragment shader"; break;
            case GL_GEOMETRY_SHADER: shaderType = "Geometry shader"; break;
            default: shaderType = "Unknown shader";
        }
        throw std::runtime_error(shaderType + " compilation failed:\n" + std::string(infoLog));
    }

    return shader;
}

GLuint Shader::linkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint geometryShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    if (geometryShader != 0)
    {
        glAttachShader(program, geometryShader);
    }

    glLinkProgram(program);

    // 检查链接错误
    int success;
    char infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        throw std::runtime_error("Shader program link failed:\n" + std::string(infoLog));
    }

    return program;
}

void Shader::use() const
{
    glUseProgram(ID);
}

GLint Shader::getUniformLocation(const std::string& name) const
{
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location == -1)
    {
        std::cerr << "[Shader Warning] Uniform not found: " << name << std::endl;
    }
    return location;
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

