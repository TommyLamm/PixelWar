@echo off
chcp 65001 >nul
echo ========================================
echo 测试程序运行
echo ========================================
cd build
echo.
echo 检查可执行文件...
if exist PixelWar.exe (
    echo [✓] 可执行文件存在
    dir PixelWar.exe
) else (
    echo [✗] 可执行文件不存在！
    exit /b 1
)

echo.
echo 检查 DLL 文件...
if exist glfw3.dll (
    echo [✓] glfw3.dll 存在
) else (
    echo [✗] glfw3.dll 不存在！
    echo 尝试从 vcpkg 复制...
    if exist ..\vcpkg\installed\x64-windows\bin\glfw3.dll (
        copy ..\vcpkg\installed\x64-windows\bin\glfw3.dll .
        echo [✓] DLL 已复制
    )
)

echo.
echo ========================================
echo 启动程序 (5秒后自动关闭)
echo ========================================
echo 如果看到窗口打开并显示深灰色背景，说明程序运行正常
echo 按 ESC 键可以退出程序
echo.
timeout /t 2 /nobreak >nul
start PixelWar.exe
echo 程序已启动，请检查窗口是否打开
echo 等待 5 秒后检查进程...
timeout /t 5 /nobreak >nul
tasklist | findstr /i "PixelWar" >nul
if %errorlevel% == 0 (
    echo [✓] 程序正在运行
) else (
    echo [✗] 程序可能已退出或未启动
)
echo.
echo 测试完成

