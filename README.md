# cs2-esp

External CS2 ESP (box + health + armor + name + distance, team-colored) with DX11 transparent overlay.

- Target: `cs2.exe` / `client.dll`
- Output: `cs2esp.exe`
- Offsets: `src/offsets.hpp` (dumper date 2026-09-10, a2x/cs2-dumper)
- Keys: `END` exit, `INS` toggle teammates

## Build (Visual Studio 2022 + Windows SDK)

```powershell
cmake -B build -A x64
cmake --build build --config Release
# -> build/Release/cs2esp.exe
```

## Build (MinGW, what produced the checked-in exe)

```powershell
cmake -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
# -> build/cs2esp.exe
```

Run as admin, CS2 windowed / fullscreen-windowed, then start `cs2esp.exe`.

If ESP is empty after a CS2 update, regenerate offsets with
https://github.com/a2x/cs2-dumper and update `src/offsets.hpp`.
