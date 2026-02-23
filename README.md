# NES.cpp

NES.cpp - простой и легковесный эмулятор с поддержкой модульности мапперов

## Скриншоты
<img src="img/kirby.png" alt="kirby" width="256" height="240">
<img src="img/smb3.png" alt="smb3" width="256" height="240">
<img src="img/dk.png" alt="dk" width="256" height="240">

## Сборка
Зависимости для сборки:
* LuaJit
* Qt6
* GCC
* CMake (3.23+)


**Linux/MacOS:**
```bash
git clone https://github.com/Noxikly/NES.cpp
cd NES.cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```