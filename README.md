# OpenGL Base Rendering Framework

A from-scratch OpenGL rendering framework built with C++, GLFW, and GLAD. It serves as a baseline for building a Left 4 Dead 2–style FPS sandbox with procedural terrain and basic combat systems.

## Features
- 1280x720 window creation (runtime uses fixed 16:9 at 1600x900)
- Game loop with dark gray clear color (0.2, 0.2, 0.2)
- ESC key quits or toggles the pause menu
- Pause menu exposes sensitivity/FOV sliders; values persist to `settings.ini`
- FPS camera with gravity, jump, and AABB collision/slide
- Enemy AI pool plus lightweight director; ray-based shooting with spread and fire-rate limit
- Procedural terrain via Perlin FBM with biomes (water/sand/grass/dirt/stone/snow) and trees
- Instanced cube rendering for terrain; standard shader for dynamic objects
- Crosshair UI and bullet trail rendering
- Structured logging and modular code for easy extension

## Dependencies
1. **GLFW 3.3+** – windowing and input  
   Download: https://www.glfw.org/download.html or install via a package manager.
2. **GLAD** – OpenGL function loader  
   Generate at https://glad.dav1d.de/ with OpenGL 4.6, Core Profile. Place `glad.c` in `src/` and headers in `include/glad/`.
3. **CMake 3.16+** – build system

## Install & build (Windows, vcpkg example)
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
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
