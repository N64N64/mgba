## How to build

### Mac / Linux

1. Install LuaJIT
2. Install [aite](https://github.com/N64N64/aite)
3. Run `aite`

### Windows

1. Install LuaJIT
2. Unzip [aite](https://github.com/N64N64/aite) to somewhere on your computer
3. Open cmd.exe, `cd` into this repo directory, and run `luajit.exe C:\PATH\TO\THE\UNZIPPED\FOLDER\main.lua`

### 3DS

Install devkitARM, then run this:

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../src/platform/3ds/CMakeToolchain.txt
make
```
