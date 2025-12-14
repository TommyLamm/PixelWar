# OpenGL FPS Sandbox (Procedural Terrain)

From-scratch OpenGL sandbox built with C++/GLFW/GLAD. It targets a Left 4 Dead 2–style FPS loop with procedural “Minecraft-like” terrain, instanced rendering, and lightweight enemy AI.

## Highlights
- FPS camera with gravity, jump, AABB collision/slide, sensitivity & FOV that persist to `settings.ini`
- Enemy pool + director with ray-based shooting (spread + fire rate), bullet trails, crosshair UI
- Procedural terrain (Perlin FBM) with simple biomes and trees; instanced cube rendering for terrain
- Streaming chunks with front-priority scheduling, batched rebuilds, and pre-sized instance buffers to reduce stutter (inspired by Sodium-style chunk handling)
- Simple dark-gray default clear, pause menu (ESC), structured logging

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
- Chunk streaming: front-first queueing, capped merges per frame, and delayed instance-buffer rebuilds to smooth hitching
- Terrain: surface-only voxels (plus water/trees) to minimize instance count
- Shooting uses spatial hash + AABB raycast; bullet trails fade quickly

## License
For learning and research use only.
