# OpenGL 基础渲染框架

这是一个从零开始构建的 OpenGL 渲染框架，使用 C++、GLFW 和 GLAD 实现。这是构建类似《求生之路2》第一人称射击游戏的基础框架。

## 功能特性

- ✅ 1280x720 窗口创建
- ✅ 游戏主循环 (Game Loop)
- ✅ 深灰色背景清除 (RGB: 0.2, 0.2, 0.2)
- ✅ ESC 键退出功能
- ✅ 完善的错误处理和日志输出
- ✅ 模块化代码结构，便于扩展

## 依赖项

### 必需库

1. **GLFW 3.3+** - 窗口和输入管理
   - 下载地址: https://www.glfw.org/download.html
   - 或使用包管理器安装

2. **GLAD** - OpenGL 函数加载器
   - 在线生成器: https://glad.dav1d.de/
   - 配置: OpenGL 4.6, Core Profile
   - 生成后，将 `glad.c` 放入 `src/` 目录
   - 将 `glad/` 头文件目录放入 `include/` 目录

3. **CMake 3.16+** - 构建系统

### 安装依赖 (Windows)

#### 使用 vcpkg (推荐)

```powershell
# 安装 vcpkg (如果还没有)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装 GLFW
.\vcpkg install glfw3:x64-windows

# 配置 CMake 使用 vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg根目录]/scripts/buildsystems/vcpkg.cmake
```

#### 手动安装

1. 下载 GLFW 预编译库: https://www.glfw.org/download.html
2. 解压到项目目录或系统路径
3. 在 CMakeLists.txt 中设置 `GLFW_DIR` 变量

### 安装依赖 (Linux)

```bash
# Ubuntu/Debian
sudo apt-get install libglfw3-dev libgl1-mesa-dev

# Fedora
sudo dnf install glfw-devel mesa-libGL-devel
```

### 安装依赖 (macOS)

```bash
brew install glfw
```

## 项目结构

```
game/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 项目说明文档
├── src/
│   ├── main.cpp            # 主程序入口
│   └── glad.c              # GLAD 生成的源文件 (需要自行生成)
└── include/
    └── glad/               # GLAD 头文件 (需要自行生成)
        ├── glad.h
        └── khrplatform.h
```

## 编译和运行

### Windows (Visual Studio)

```powershell
# 创建构建目录
mkdir build
cd build

# 配置项目 (使用 Visual Studio 2019 或更高版本)
cmake .. -G "Visual Studio 16 2019" -A x64

# 编译
cmake --build . --config Release

# 运行
.\Release\OpenGLRenderFramework.exe
```

### Linux/macOS

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 编译
make

# 运行
./OpenGLRenderFramework
```

## GLAD 设置步骤

1. 访问 https://glad.dav1d.de/
2. 选择以下配置:
   - **Language**: C/C++
   - **Specification**: OpenGL
   - **API gl**: Version 4.6
   - **Profile**: Core
   - **Generate a loader**: ✅ 勾选
3. 点击 **Generate** 下载 ZIP 文件
4. 解压后:
   - 将 `src/glad.c` 复制到项目的 `src/` 目录
   - 将 `include/glad/` 目录复制到项目的 `include/` 目录

## 使用说明

1. 运行程序后，会创建一个 1280x720 的窗口
2. 窗口背景为深灰色 (RGB: 0.2, 0.2, 0.2)
3. 按 **ESC** 键或点击窗口关闭按钮可退出程序
4. 控制台会输出详细的初始化信息和 GPU 信息

## 后续扩展方向

- [ ] 着色器系统 (Shader System)
- [ ] 顶点缓冲和索引缓冲 (VBO/IBO)
- [ ] 矩阵变换库 (GLM)
- [ ] 纹理加载系统
- [ ] 相机系统 (第一人称/第三人称)
- [ ] 输入系统 (键盘/鼠标)
- [ ] 资源管理器
- [ ] 场景图系统
- [ ] 物理引擎集成

## 故障排除

### 编译错误: 找不到 GLFW

- 确保已正确安装 GLFW
- 检查 CMakeLists.txt 中的 `find_package(glfw3)` 是否能找到库
- 尝试手动设置 `GLFW_DIR` 变量

### 运行时错误: GLAD 初始化失败

- 确保 `glad.c` 已正确添加到项目中
- 检查 `glad/glad.h` 头文件路径是否正确
- 确保在调用 `gladLoadGLLoader` 之前已调用 `glfwMakeContextCurrent`

### 窗口创建失败

- 检查显卡驱动是否已更新
- 确认系统支持 OpenGL 3.3 或更高版本
- 在 Linux 上，确保安装了正确的 OpenGL 驱动

## 许可证

本项目仅供学习和研究使用。

## 作者

游戏引擎架构组

