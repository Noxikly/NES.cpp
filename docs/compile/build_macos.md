# Компиляция под MacOS

## Зависимости
* GCC v6+ (для поддержки C++ 17)
* CMake 3.22+
* LuaJIT 2.1
* Qt6


Все вышеперечисленные зависимости могут быть установленны одной командой:\
`brew install luajit cmake gcc qt`

## Компиляция
Скопируйте репозиторий
``` bash
git clone https://github.com/Noxikly/NES.cpp
cd NES.cpp
```

Затем скомпилируйте Release версию (Или Debug, если это необходимо)
``` bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```