cmake_minimum_required(VERSION 3.20)
project(PixelWar)

# 1. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 寻找 vcpkg 安装的库
# CONFIG 模式能更好地配合 vcpkg
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# 3. 添加可执行文件
# 注意：这里不需要 src/glad.c，因为我们链接的是库
# 确保你的 main.cpp 在 src 文件夹里，或者根据实际位置修改路径
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/Camera.cpp
    src/Shader.cpp
    src/Mesh.cpp
    src/Geometry.cpp
    src/Enemy.cpp
    src/EnemyPool.cpp
    src/AIDirector.cpp
    src/InstancedMesh.cpp
    src/Settings.cpp
)

# 4. 链接库 (关键步骤)
# vcpkg 会自动处理头文件路径，所以不需要 target_include_directories
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glad::glad      # 链接 GLAD
    glfw            # 链接 GLFW (vcpkg通常暴露为 glfw)
    glm::glm        # 链接 GLM
)

# 5. 简单的编译选项优化 (可选)
if(MSVC)
    # 设置 UTF-8 编码，解决 C4819 警告（中文注释和字符串）
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

# 6. 复制资源文件到输出目录
# 将项目根目录下的 shaders 文件夹复制到可执行文件所在的目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)