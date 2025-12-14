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
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
# run
.\Release\PixelWar.exe
```

## Build (Linux/macOS)
```bash
mkdir build && cd build
cmake ..
cmake --build .
./PixelWar
```

## GLAD setup
Generate at https://glad.dav1d.de/ with:
- Language: C/C++
- API: OpenGL 4.6, Core Profile
- Generate a loader: enabled
Then place `src/glad.c` into `src/` and headers into `include/glad/`.

## Controls & notes
- WASD move, Space jump, Mouse look, LMB fire, ESC pause/resume
- Pause menu shows sensitivity/FOV (progress bars); values persist to `settings.ini`
- Shooting uses spatial grid + AABB raycast; bullet trails fade quickly

## License
For learning and research use only.

---

## 繁體中文說明

Pixel War 是一個使用 C++ / OpenGL（GLFW + GLAD + GLM）的方塊 FPS 沙盒示例，包含程序化地形（Perlin FBM + 生物群系 + 樹木）、方塊實例化渲染、具有物理與跳躍的 FPS 攝影機、敵人 AI（物件池與導演）、射擊（空間網格加速射線、彈道軌跡）、暫停選單與設定保存。

### 功能重點
- 固定 16:9 窗口 (1600x900) 與暫停選單
- FPS 攝影機：重力、跳躍、AABB 碰撞與滑動
- 敵人：追蹤 + 分離，物件池，簡易導演
- 射擊：空間網格加速射線，彈道軌跡，散布與射速限制
- 程序化地形：Perlin FBM、生物群系（海/沙/草/土/石/雪）、樹木
- 渲染：地形實例化著色器，動態物件標準著色器，準星 UI
- 設定：靈敏度、FOV 可在暫停選單調整並寫入 `settings.ini`

### 編譯 (Windows 範例)
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
.\Release\PixelWar.exe
```

### 編譯 (Linux/macOS)
```bash
mkdir build && cd build
cmake ..
cmake --build .
./PixelWar
```

### GLAD 設定
於 https://glad.dav1d.de/ 生成：
- Language: C/C++
- API: OpenGL 4.6, Core Profile
- Generate a loader: 勾選
將 `src/glad.c` 放入 `src/`，標頭放入 `include/glad/`。

### 操作說明
- WASD 移動，空白鍵跳躍，滑鼠視角，左鍵開火，ESC 暫停/繼續
- 暫停選單內可調整靈敏度/FOV（有進度條），並寫入 `settings.ini`
- 射擊使用空間網格與 AABB 射線檢測；彈道軌跡快速淡出

### 授權
僅供學習與研究使用。
